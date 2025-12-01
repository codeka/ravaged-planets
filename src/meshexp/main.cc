#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <framework/bitmap.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/model_writer.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>

namespace po = boost::program_options;

//-----------------------------------------------------------------------------

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

bool export_scene(aiScene const *scene, std::string const &filename);
bool add_mesh(fw::model &mdl, aiMesh *mesh);
std::shared_ptr<fw::model_node> add_node(fw::model &mdl, aiNode *node, int level);

//-----------------------------------------------------------------------------

void meshexp(std::string input_filename, std::string output_filename) {
  Assimp::Importer importer;

  fw::debug << "reading file: " << input_filename << std::endl;

  // set the list of things we want the importer to ignore (COLORS is the most important)
  importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
      aiComponent_TANGENTS_AND_BITANGENTS | aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);

  // set the maximum number of vertices and faces we'll put in a single mesh
  importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 35535);
  importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 35535);

  // set the maximum number of bones we want per vertex
  importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);

  // import the scene!
  aiScene const *scene = importer.ReadFile(input_filename.c_str(),
      aiProcess_Triangulate | aiProcess_JoinIdenticalVertices /*aiProcess_LimitBoneWeights */
      | aiProcess_SortByPType | aiProcess_RemoveComponent | aiProcess_SplitLargeMeshes
          | aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials | aiProcess_GenUVCoords
          | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_GenNormals | aiProcess_FlipWindingOrder
          | aiProcess_ValidateDataStructure | aiProcess_FindInvalidData);

  if (scene == nullptr) {
    fw::debug << "-- ERROR --" << std::endl;
    fw::debug << importer.GetErrorString() << std::endl;
    return;
  }

  export_scene(scene, output_filename);
}

bool export_scene(aiScene const *scene, std::string const &filename) {
  fw::debug << "- writing file: " << filename << std::endl;

  // build up the fw::model first, and then use the fw::model_writer to
  // write it to disk.
  fw::model mdl;
  for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    if (!add_mesh(mdl, scene->mMeshes[i]))
      return false;
  }

  // add the root node (and recursively find all it's children as well)
  mdl.root_node = add_node(mdl, scene->mRootNode, 0);

  if (scene->mAnimations != 0) {
    // add animations
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      fw::debug << "  adding animation: (\"" << scene->mAnimations[0]->mName.data << "\")" << std::endl;
    }
  }

  fw::model_writer writer;
  writer.write(filename, mdl);

  return true;
}

// adds the given aiMesh to the given fw::model
bool add_mesh(fw::model &mdl, aiMesh *mesh) {
  fw::debug << "  adding mesh (" << mesh->mNumBones << " bone(s), " << mesh->mNumVertices << " vertex(es), "
      << mesh->mNumFaces << " face(s))" << std::endl;

  // create a vector of xyz_n_uv vertices and set it to zero initially
  std::shared_ptr<fw::model_mesh_noanim> mm(new fw::model_mesh_noanim(mesh->mNumVertices, mesh->mNumFaces * 3));
  memset(&mm->vertices[0], 0, sizeof(fw::vertex::xyz_n_uv) * mesh->mNumVertices);
  memset(&mm->indices[0], 0, sizeof(uint16_t) * mesh->mNumFaces * 3);

  // copy each of the vertices from the mesh into our own array
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    mm->vertices[i].x = mesh->mVertices[i].x;
    mm->vertices[i].y = mesh->mVertices[i].y;
    mm->vertices[i].z = mesh->mVertices[i].z;
    if (mesh->mNormals != 0) {
      mm->vertices[i].nx = mesh->mNormals[i].x;
      mm->vertices[i].ny = mesh->mNormals[i].y;
      mm->vertices[i].nz = mesh->mNormals[i].z;
    }
    // we currently only support one set of texture coords
    if (mesh->mTextureCoords[0] != 0) {
      mm->vertices[i].u = mesh->mTextureCoords[0][i].x;
      mm->vertices[i].v = 1.0f - mesh->mTextureCoords[0][i].y;
    }
  }

  // copy each of the faces from the mesh into our array, since we passed aiProcess_Triangulate,
  // each face should be a nice triangle for us
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    if (mesh->mFaces[i].mNumIndices != 3) {
      fw::debug << "PANIC! face is not a triangle! mNumIndices = " << mesh->mFaces[i].mNumIndices << std::endl;
      return false;
    }

    for (int j = 0; j < 3; j++) {
      if (mesh->mFaces[i].mIndices[j] > 0xffff) {
        fw::debug << "PANIC! face index is bigger than that supported by a 16-bit value: "
            << mesh->mFaces[i].mIndices[j] << std::endl;
        return false;
      }

      mm->indices[i * 3 + j] = static_cast<uint16_t>(mesh->mFaces[i].mIndices[j]);
    }
  }

  mdl.meshes.push_back(std::dynamic_pointer_cast<fw::model_mesh>(mm));

  return true;
}

std::shared_ptr<fw::model_node> add_node(fw::model &mdl, aiNode *node, int level) {
  fw::debug << "  " << std::string(level * 2, ' ') << "adding node \"" << node->mName.data << "\"" << " ("
      << node->mNumChildren << " child(ren), " << node->mNumMeshes << " meshe(s))" << std::endl;
  std::shared_ptr<fw::model_node> root_node;
  if (node->mNumMeshes == 1) {
    // if there's just one mesh (this is the most common case), then we just copy
    // the vertex_buffer, index_buffer and so on to the scenegraph node
    root_node = std::shared_ptr<fw::model_node>(new fw::model_node());
    root_node->mesh_index = node->mMeshes[0];
  } else {
    root_node = std::shared_ptr<fw::model_node>(new fw::model_node());
    root_node->mesh_index = -1;

    // we create one child node for each of our meshes
    for (unsigned int index = 0; index < node->mNumMeshes; index++) {
      std::shared_ptr<fw::model_node> child_node(new fw::model_node());
      child_node->mesh_index = node->mMeshes[index];
      root_node->add_child(std::dynamic_pointer_cast<fw::sg::node>(child_node));
    }
  }

  // copy the matrix from the aiNode to our new node as well
  fw::matrix matrix;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      matrix(i, j) = node->mTransformation[j][i];
    }
  }

  // add the mesh_scale factor, if there is one...
//  if (mesh_scale != 1.0f) {
//    matrix = fw::scale(mesh_scale) * matrix;
//  }

  root_node->transform = matrix;

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    std::shared_ptr<fw::model_node> child_node = add_node(mdl, node->mChildren[i], level + 1);
    root_node->add_child(std::dynamic_pointer_cast<fw::sg::node>(child_node));
  }

  return root_node;
}

//-----------------------------------------------------------------------------

class LogStream : public Assimp::LogStream {
public:
  void write(const char* message) {
    std::string msg(message);
    boost::trim(msg);
    fw::debug << " assimp : " << msg << std::endl;
  }
};

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    Assimp::DefaultLogger::create(nullptr, Assimp::Logger::VERBOSE, 0, nullptr);
    Assimp::DefaultLogger::get()->attachStream(new LogStream());

    fw::tool_application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Meshexp");

    fw::settings stg;
    meshexp(stg.get_value<std::string>("input"), stg.get_value<std::string>("output"));
  } catch (std::exception &e) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << e.what() << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }

  return 0;
}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
}

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");
  options.add_options()("input", po::value<std::string>()->default_value(""), "The input file to load, must be a supported mesh type.");
  options.add_options()("output", po::value<std::string>()->default_value(""), "The output file to save as, must end with '.mesh'.");

  fw::settings::initialize(options, argc, argv, "meshexp.conf");
}
