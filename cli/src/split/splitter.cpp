#include "./splitter.h"


void splitter::initBVH(GroupObject &group, int level = 0, bool shouldDivideVertical = false) {
  if (level < 8) {
    Vector3f size = group->boundingBox.size();

    if (shouldDivideVertical) {
      GroupObject groupTop = GroupObject(new Group());
      GroupObject groupBottom = GroupObject(new Group());

      groupTop->boundingBox.min = group->boundingBox.min.clone();
      groupTop->boundingBox.max = group->boundingBox.max.clone();
      groupTop->boundingBox.max.y -= size.y * 0.5f;

      groupBottom->boundingBox.min = group->boundingBox.min.clone();
      groupBottom->boundingBox.max = group->boundingBox.max.clone();
      groupBottom->boundingBox.min.y += size.y * 0.5f;

      group->children.push_back(groupTop);
      group->children.push_back(groupBottom);

      splitter::initBVH(groupTop, level + 1, !shouldDivideVertical);
      splitter::initBVH(groupBottom, level + 1, !shouldDivideVertical);
    } else {
      GroupObject groupLeft = GroupObject(new Group());
      GroupObject groupRight = GroupObject(new Group());

      groupLeft->boundingBox.min = group->boundingBox.min.clone();
      groupLeft->boundingBox.max = group->boundingBox.max.clone();
      groupLeft->boundingBox.max.x -= size.x * 0.5f;

      groupRight->boundingBox.min = group->boundingBox.min.clone();
      groupRight->boundingBox.max = group->boundingBox.max.clone();
      groupRight->boundingBox.min.x += size.x * 0.5f;

      group->children.push_back(groupLeft);
      group->children.push_back(groupRight);

      splitter::initBVH(groupLeft, level + 1, !shouldDivideVertical);
      splitter::initBVH(groupRight, level + 1, !shouldDivideVertical);
    }
  } else if (level == 8) {
    MeshObject mesh = MeshObject(new Mesh());
    mesh->name = 
      std::string("uv_") +
      std::to_string(group->boundingBox.min.x) + std::string(",") + std::to_string(group->boundingBox.min.y) +
      std::string("_") +
      std::to_string(group->boundingBox.max.x) + std::string(",") + std::to_string(group->boundingBox.max.y);
    mesh->material.name = mesh->name;
    group->meshes.push_back(mesh);
  }
};

GroupObject splitter::splitUV(GroupObject baseObject) {
  baseObject->computeUVBox();

  // Split meshes by material name
  std::map<std::string, GroupObject> meshMaterialMap;
  //std::map<std::string, Material> materialMap;

  bool lastGroup = false;
  GroupObject currentGroup;

  bool ix, iy, iz;

  baseObject->traverse([&](MeshObject mesh){
    if (mesh->material.name != "") {
      GroupObject group;

      // Create group if doesn't exist
      if (meshMaterialMap.find(mesh->material.name) == meshMaterialMap.end()) {
        group = GroupObject(new Group());
        group->meshes.push_back(MeshObject(new Mesh()));

        group->meshes[0]->material = mesh->material;
        group->meshes[0]->material.diffuseMapImage = mesh->material.diffuseMapImage;

        group->boundingBox.min.set(0.0f, 0.0f, 0.0f);
        group->boundingBox.max.set(1.0f, 1.0f, 0.0f);

        meshMaterialMap[mesh->material.name] = group;
        //materialMap[mesh->material.name] = mesh->material;

        // Init full BVH tree with 3 inherite subtrees
        splitter::initBVH(group);
      } else {
        group = meshMaterialMap[mesh->material.name];
      }

      group->meshes[0]->position.insert(
        std::end(group->meshes[0]->position),
        std::begin(mesh->position),
        std::end(mesh->position)
      );

      group->meshes[0]->normal.insert(
        std::end(group->meshes[0]->normal),
        std::begin(mesh->normal),
        std::end(mesh->normal)
      );

      group->meshes[0]->uv.insert(
        std::end(group->meshes[0]->uv),
        std::begin(mesh->uv),
        std::end(mesh->uv)
      );

      for (Face &face : mesh->faces) // access by reference to avoid copying
      {
        lastGroup = false;
        currentGroup = group;

        // Get latest BVH subgroup from top to down
        while (!lastGroup) {
          if (currentGroup->children.size() == 0) {
            lastGroup = true;

            currentGroup->meshes[0]->faces.push_back(face);
          } else {
            ix = currentGroup->children[0]->boundingBox.intersect(mesh->uv[face.uvIndices[0]]);
            iy = currentGroup->children[0]->boundingBox.intersect(mesh->uv[face.uvIndices[1]]);
            iz = currentGroup->children[0]->boundingBox.intersect(mesh->uv[face.uvIndices[2]]);

            if (ix || iy || iz) {
              currentGroup = currentGroup->children[0];
            } else {
              currentGroup = currentGroup->children[1];
            }
          }
        }
      }
    }
  });

  GroupObject resultGroup = GroupObject(new Group());
  resultGroup->name = baseObject->name;

  int meshIndex = 0;
  for (std::map<std::string, GroupObject>::iterator it = meshMaterialMap.begin(); it != meshMaterialMap.end(); ++it) {
    it->second->traverse([&](MeshObject mesh){
      //std::cout << "Mesh \"" << mesh->name << "\" has: " << mesh->faces.size() << " faces" << std::endl;
      mesh->hasNormals = it->second->meshes[0]->normal.size() > 0;
      mesh->hasUVs = it->second->meshes[0]->uv.size() > 0;
      mesh->remesh(it->second->meshes[0]->position, it->second->meshes[0]->normal, it->second->meshes[0]->uv);
      mesh->finish();

      mesh->material = it->second->meshes[0]->material;
      mesh->material.name += std::to_string(meshIndex);
      mesh->material.diffuseMap = mesh->material.name + ".jpg";
      //mesh->material.diffuseMapImage = it->second->meshes[0]->material.diffuseMapImage;

      meshIndex++;

      if (mesh->hasUVs) {
        BBoxf uvBox = mesh->uvBox;
        // uvBox.fromPoint(mesh->uv[0].x, mesh->uv[0].y, 0.0f);

        // for (long long unsigned int i = 1; i < mesh->uv.size(); i++) {
        //   uvBox.extend(mesh->uv[i].x, mesh->uv[i].y, 0.0f);
        // }

        int minX, minY, maxX, maxY;

        //Material meshMaterial = materialMap[it->first];

        minX = floor(uvBox.min.x * it->second->meshes[0]->material.diffuseMapImage.width);
        minY = floor(uvBox.min.y * it->second->meshes[0]->material.diffuseMapImage.height);
        maxX = ceil(uvBox.max.x * it->second->meshes[0]->material.diffuseMapImage.width);
        maxY = ceil(uvBox.max.y * it->second->meshes[0]->material.diffuseMapImage.height);

        //std::cout << "Calculated Box min x/y: " << minX << "/" << minY << " max x/y: " << maxX << "/" << maxY << std::endl;
        //std::cout << "Real Box min x/y: " << uvBox.min.x << "/" << uvBox.min.y << " max x/y: " << uvBox.max.x << "/" << uvBox.max.y << std::endl;

        float offsetX1 = (float) minX / (float) it->second->meshes[0]->material.diffuseMapImage.width;
        float offsetX2 = (float) maxX / (float) it->second->meshes[0]->material.diffuseMapImage.width;

        float offsetY1 = (float) minY / (float) it->second->meshes[0]->material.diffuseMapImage.height;
        float offsetY2 = (float) maxY / (float) it->second->meshes[0]->material.diffuseMapImage.height;

        float UVWidth = offsetX2 - offsetX1;
        float UVHeight = offsetY2 - offsetY1;

        for (long long unsigned int i = 0; i < mesh->uv.size(); i++) {
          // uvBox.extend(mesh->uv[i].x, mesh->uv[i].y, 0.0f);
          mesh->uv[i].x = (mesh->uv[i].x - offsetX1) / UVWidth;
          mesh->uv[i].y = (mesh->uv[i].y - offsetY1) / UVHeight;
        }
        
        int textureWidth = maxX - minX;
        int textureHeight = maxY - minY;

        // Image &diffuse = mesh->material.diffuseMapImage;
        Image diffuse;

        diffuse.channels = 3;
        diffuse.width = textureWidth;
        diffuse.height = textureHeight;
        diffuse.data = new unsigned char[textureWidth * textureHeight * 3];
        
        
        for (int j = 0; j < textureWidth; j++)
        {
          for (int i = 0; i < textureHeight; i++)
          {
            int originPointer = ((it->second->meshes[0]->material.diffuseMapImage.height - 1 - i - minY) * it->second->meshes[0]->material.diffuseMapImage.width * diffuse.channels) + ((j + minX) * diffuse.channels);
            int targetPointer = ((textureHeight - 1 - i) * diffuse.width * diffuse.channels) + (j * diffuse.channels);

            diffuse.data[targetPointer] = it->second->meshes[0]->material.diffuseMapImage.data[originPointer];
            diffuse.data[targetPointer + 1] = it->second->meshes[0]->material.diffuseMapImage.data[originPointer + 1];
            diffuse.data[targetPointer + 2] = it->second->meshes[0]->material.diffuseMapImage.data[originPointer + 2];
          }
        }

        mesh->material.diffuseMapImage = diffuse;
      }

      // std::cout << "Mesh \"" << mesh->name << "\" has: " << mesh->faces.size() << " faces, " << mesh->position.size() << " vertices" << std::endl;

      if (mesh->faces.size() > 0) {
        // std::cout << "Mesh is ready for saving: " << mesh->name << std::endl;
        resultGroup->meshes.push_back(mesh);
        resultGroup->computeUVBox();
        resultGroup->computeBoundingBox();
      }
    });

    // std::cout << "Group finished: " << it->first << std::endl;
    //it->second->meshes[0]->material = NULL;
  }

  // std::cout << "All groups are finished" << std::endl;

  for (std::map<std::string, GroupObject>::iterator it = meshMaterialMap.begin(); it != meshMaterialMap.end(); ++it) {
    it->second->meshes[0]->free();
    it->second->meshes.clear();
  }

  // baseObject->traverse([](MeshObject mesh) {
  //   mesh->free();
  // });

  return resultGroup;
};

bool splitter::splitObject(GroupObject baseObject, GroupCallback fn, bool isVertical = false) {
  unsigned int polygonCount = 0;

  baseObject->traverse([&](MeshObject mesh){
    polygonCount += mesh->faces.size();
  });

  //std::cout << "Total polygons: " << polygonCount << std::endl;

  // fn(baseObject);

  if (polygonCount <= 20000) {
    baseObject->name = "Chunk_";
    fn(splitter::splitUV(baseObject));

    return false;
  }

  float xMedian = 0.0f;
  float zMedian = 0.0f;
  unsigned int verticesCount = 0;

  baseObject->traverse([&](MeshObject mesh){
    for (Vector3f &position : mesh->position) // access by reference to avoid copying
    {
      if (isVertical) {
        xMedian += position.x;
      } else {
        zMedian += position.z;
      }
    }

    verticesCount += mesh->position.size();
  });

  if (verticesCount == 0) {
    verticesCount = 1;
  }

  xMedian /= verticesCount;
  zMedian /= verticesCount;

  GroupObject left = GroupObject(new Group());
  GroupObject right = GroupObject(new Group());


  Vector3f pos1, pos2, pos3;
  // unsigned int leftFaces = 0;
  // unsigned int rightFaces = 0;

  bool isLeft = false;

  baseObject->traverse([&](MeshObject mesh){
    MeshObject leftMesh = MeshObject(new Mesh());
    MeshObject rightMesh = MeshObject(new Mesh());

    for (Face &face : mesh->faces) // access by reference to avoid copying
    {
      pos1 = mesh->position[face.positionIndices[0]];
      pos2 = mesh->position[face.positionIndices[1]];
      pos3 = mesh->position[face.positionIndices[2]];

      if (isVertical) {
        isLeft = pos1.x < xMedian || pos2.x < xMedian || pos3.x < xMedian;
        if (isLeft) {
          leftMesh->faces.push_back(face);
        } else {
          rightMesh->faces.push_back(face);
        }
      } else {
        isLeft = pos1.z < zMedian || pos2.z < zMedian || pos3.z < zMedian;
        if (isLeft) {
          leftMesh->faces.push_back(face);
        } else {
          rightMesh->faces.push_back(face);
        }
      }
    }

    if (leftMesh->faces.size() != 0) {
      leftMesh->name = mesh->name;
      leftMesh->material = mesh->material;
      leftMesh->hasNormals = mesh->hasNormals;
      leftMesh->hasUVs = mesh->hasUVs;

      leftMesh->remesh(mesh->position, mesh->normal, mesh->uv);

      left->meshes.push_back(leftMesh);
    }

    if (rightMesh->faces.size() != 0) {
      rightMesh->name = mesh->name;
      rightMesh->material = mesh->material;
      rightMesh->hasNormals = mesh->hasNormals;
      rightMesh->hasUVs = mesh->hasUVs;

      rightMesh->remesh(mesh->position, mesh->normal, mesh->uv);

      right->meshes.push_back(rightMesh);
    }
  });

  if (left->meshes.size() != 0) {
    splitter::splitObject(left, fn, !isVertical);
  }

  if (right->meshes.size() != 0) {
    splitter::splitObject(right, fn, !isVertical);
  }

  return true;
};

void splitter::splitObject(GroupObject baseObject, GroupCallback fn) {
  splitter::splitObject(baseObject, fn, true);
  //fn(splitter::splitUV(group));
};