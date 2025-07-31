#include "scene/scene.h"

bool loadGltfScene(Scene& _scene, const char* _path, bool _buildMeshlet, bool _seamlessLod);

bool loadGltfMesh(Geometry& _geo, const char* _path, bool _buildMeshlet, bool _seamlessLod);