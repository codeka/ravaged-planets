#pragma once

#include <vector>

#include <framework/math.h>

namespace fw {
struct PathNode;

// Class for finding a path between point "A" and point "B" in a grid.
class PathFind {
private:
  int _width;
  int _length;
  PathNode *_nodes;
  int _run_no;

  PathNode *get_node(fw::Vector const &loc) const;
  bool is_passable(fw::Vector const &start, fw::Vector const &end) const;

public:
  PathFind(int width, int length, std::vector<bool> const &passability);
  virtual ~PathFind();

  // Finds a path between the given 'start' and 'end' vectors. We ignore the y component of the vectors and just look
  // at the (x,y) components. The 'path' is populated with the path we found and we'll assume the agent will travel
  // between it's current location and the first element of path, then to the second element and so on traveling in a
  // straight line each time.
  //
  // Returns true if a path was found, false if no path exists
  virtual bool find(std::vector<fw::Vector> &path, fw::Vector const &start, fw::Vector const &end);

  // Simplifies the given path by removing any unneeded nodes. For example, if you have an "L" shaped path where a
  // straight line will do, this will turn the path into a straight line. Also, if you have lots of nodes in a straight
  // line, this will remove all but the first & last, etc.
  //
  // Note: the path in full_path is not modified, but the simplified path is built up in new_path.
  virtual void simplify_path(std::vector<fw::Vector> const &full_path, std::vector<fw::Vector> &new_path);
};

// This specialization of PathFfind adds some timing methods and it also keeps the final Node structure around
// for visualization
class TimedPathFind: public PathFind {
public:
  TimedPathFind(int width, int length, std::vector<bool> &passability);
  virtual ~TimedPathFind();

  // the total time the last find() call took, in seconds.
  float total_time;

  virtual bool find(std::vector<fw::Vector> &path, fw::Vector const &start, fw::Vector const &end);
};

}
