#include <set>

#include <framework/path_find.h>
#include <framework/timer.h>
#include <framework/vector.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/misc.h>

namespace fw {

struct path_node {
  path_node *previous;
  float cost_to_goal;
  float cost_from_start;
  fw::vector loc;
  bool passable;

  struct cost_comparer {
    bool operator()(path_node const *lhs, path_node const *rhs) {
      float lhs_total_cost = lhs->cost_to_goal + lhs->cost_from_start;
      float rhs_total_cost = rhs->cost_to_goal + rhs->cost_from_start;
      return lhs_total_cost < rhs_total_cost;
    }
  };

  std::multiset<path_node *, cost_comparer>::iterator open_it;
  int open_run_no;  // the run_no we were last inserted into the open set
  int closed_run_no;  // the run_no we were last inserted into the closed set
};

path_find::path_find(int width, int length, std::vector<bool> const &passability) :
    _width(width), _length(length), _run_no(0) {
  _nodes = new path_node[_width * _length];
  for (int z = 0; z < _length; z++) {
    for (int x = 0; x < _width; x++) {
      path_node &node = _nodes[(z * _width) + x];
      node.loc = fw::vector(x, 0, z);
      node.open_run_no = 0;
      node.closed_run_no = 0;
      node.passable = passability[(z * _width) + x];
    }
  }
}

path_find::~path_find() {
  delete[] _nodes;
}

float estimate_cost(fw::vector const &from, fw::vector const &to) {
  // we'll use the manhatten distance.
  return abs(from[0] - to[0]) + abs(from[2] - to[2]);
}

void construct_path(std::vector<fw::vector> &path, path_node const *goal_node) {
  path_node const *node = goal_node;
  while (node != 0) {
    path.insert(path.begin(), node->loc);
    node = node->previous;
  }
}

path_node *path_find::get_node(fw::vector const &loc) const {
  int x = fw::constrain(static_cast<int>(loc[0]), _width);
  int z = fw::constrain(static_cast<int>(loc[2]), _length);
  return &_nodes[(z * _width) + x];
}

bool path_find::find(std::vector<fw::vector> &path, fw::vector const &start, fw::vector const &end) {
  std::multiset<path_node *, path_node::cost_comparer> open_set;

  // increment the run_no (basically invalidating all the current path_nodes)
  _run_no++;

  // add the initial node to the open set
  path_node *start_node = get_node(start);
  start_node->previous = 0;
  start_node->open_run_no = _run_no;
  start_node->cost_to_goal = estimate_cost(start_node->loc, end);
  start_node->cost_from_start = 0.0f;
  start_node->open_it = open_set.insert(start_node);

  auto open_it = open_set.begin();
  while (open_it != open_set.end()) {
    // grab the first node from the open set
    path_node *curr = *open_it;

    // work out if we're at the goal
    float cost_to_goal = curr->cost_to_goal;
    if (cost_to_goal <= 1.0f) {
      construct_path(path, curr);
      return true;
    }

    // we've now processed this node, add it to the closed set
    curr->closed_run_no = _run_no;
    curr->open_run_no = 0;
    open_set.erase(curr->open_it);

    // find all the neighbours and add them to the open set
    for (int dz = -1; dz <= 1; dz++) {
      for (int dx = -1; dx <= 1; dx++) {
        // skip the current node!!
        if (dx == 0 && dz == 0)
          continue;

        // find the node, if we've already visited and discounted it,
        // don't visit it again
        path_node *n = get_node(fw::vector(curr->loc[0] + dx, 0.0f, curr->loc[2] + dz));

        // if it's in the closed list already or not passable, don't even consider it
        if (n->closed_run_no == _run_no || !n->passable)
          continue;

        // estimate the cost to the goal from this node
        float new_cost_to_goal = estimate_cost(n->loc, end);

        // the actual cost from the start: sqrt(2) for diagonals; 1 for straights
        // (that's the actual length of the line...)
        float new_cost_from_start = curr->cost_from_start + (dx == 0 || dz == 0 ? 1.0f : 1.41421356f);

        // if it's a non-visited node, or if it's cheaper to travel this path than
        // go directly from that one, then use this path instead
        if (n->open_run_no != _run_no
            || (new_cost_from_start + new_cost_to_goal) < (n->cost_from_start + n->cost_to_goal)) {
          // the next closest node to this one is the one we're already looking at
          n->previous = curr;

          n->cost_to_goal = new_cost_to_goal;
          n->cost_from_start = new_cost_from_start;

          if (n->open_run_no == _run_no) {
            // if it's already on the open list, remove it and re-add it
            // since, by definition, it will now be lower cost
            open_set.erase(n->open_it);
          } else {
            n->open_run_no = _run_no;
          }
          n->open_it = open_set.insert(n);
        }
      }
    }

    open_it = open_set.begin();
  }

  // if we get here, it means we couldn't find a path at all...
  return false;
}

bool path_find::is_passable(fw::vector const &start, fw::vector const &end) const {
  // we need to determine whether a straight line from start to end is passable
  // or not. We'll trace a line from start to end then look up all the nodes
  // in between

  // todo: we use the same algorithm here and in terrain::get_cursor_location can we factor it out?
  int sx = static_cast<int>(floor(start[0] + 0.5f));
  int sz = static_cast<int>(floor(start[2] + 0.5f));
  int ex = static_cast<int>(floor(end[0] + 0.5f));
  int ez = static_cast<int>(floor(end[2] + 0.5f));

  int dx = ex - sx;
  int dz = ez - sz;
  int abs_dx = abs(dx);
  int abs_dz = abs(dz);

  int steps = (abs_dx > abs_dz) ? abs_dx : abs_dz;

  float xinc = dx / static_cast<float>(steps);
  float zinc = dz / static_cast<float>(steps);

  float x = static_cast<float>(sx);
  float z = static_cast<float>(sz);
  for (int i = 0; i <= steps; i++) {
    path_node *node = get_node(fw::vector(x, 0, z));
    if (!node->passable)
      return false;

    x += xinc;
    z += zinc;
  }

  return true;
}

void path_find::simplify_path(std::vector<fw::vector> const &full_path, std::vector<fw::vector> &new_path) {
  std::vector<fw::vector>::const_iterator fp_it = full_path.begin();
  if (fp_it == full_path.end())
    return;

  // the first node in full_path is also the first node in new_path
  new_path.push_back(*fp_it);
  fw::vector start = new_path[0];

  ++fp_it; // move to the second node now...
  for (; fp_it != full_path.end(); ++fp_it) {
    fw::vector end = *fp_it;

    if (!is_passable(start, end)) {
      // if we can't go from start to end without passing over an impassable
      // node, then we'll have to add (fp_it-1) to the new_path and start again
      // from there

      new_path.push_back(*(fp_it - 1));
      start = new_path[new_path.size() - 1];
    }
  }

  if (new_path[new_path.size() - 1] != full_path[full_path.size() - 1]) {
    new_path.push_back(full_path[full_path.size() - 1]);
  }
}

//-------------------------------------------------------------------------

timed_path_find::timed_path_find(int width, int length, std::vector<bool> &passability) :
    path_find(width, length, passability), total_time(0) {
}

timed_path_find::~timed_path_find() {
}

bool timed_path_find::find(std::vector<fw::vector> &path, fw::vector const &start, fw::vector const &end) {
  fw::timer tmr;
  tmr.start();

  bool found = path_find::find(path, start, end);

  tmr.stop();
  total_time = tmr.get_total_time();

  return found;
}
}
