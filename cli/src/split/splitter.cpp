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

  // std::cout << "UV split has been started" << std::endl;

  baseObject->traverse([&](MeshObject mesh){
    // std::cout << "Mesh material name: " << mesh->material.name << std::endl;
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
        // std::cout << "BVH generation has been started" << std::endl;
        splitter::initBVH(group);
        // std::cout << "BVH generation has been finished" << std::endl;
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

  // std::cout << "Mesh split by material has been finished" << std::endl;

  GroupObject resultGroup = GroupObject(new Group());
  resultGroup->name = baseObject->name;

  // std::cout << "Texture split has been staretd" << std::endl;

  int meshIndex = 0;
  for (std::map<std::string, GroupObject>::iterator it = meshMaterialMap.begin(); it != meshMaterialMap.end(); ++it) {
    it->second->traverse([&](MeshObject mesh){
      // std::cout << "Current mesh material name: " << mesh->material.name << std::endl;
      // std::cout << "Mesh \"" << mesh->name << "\" has: " << mesh->faces.size() << " faces" << std::endl;

      // std::cout << "Mesh info assign" << std::endl;
      // std::cout << "Group data: " << it->second->meshes.size() << " meshes" << std::endl;
      // std::cout << it->second->meshes[0]->position.size()  << "v, " << it->second->meshes[0]->normal.size()  << "n, " << it->second->meshes[0]->uv.size()  << "t" << std::endl;

      mesh->hasNormals = it->second->meshes[0]->normal.size() > 0;
      mesh->hasUVs = it->second->meshes[0]->uv.size() > 0;
      mesh->remesh(it->second->meshes[0]->position, it->second->meshes[0]->normal, it->second->meshes[0]->uv);
      mesh->finish();

      // std::cout << "Material info assign" << std::endl;
      mesh->material = it->second->meshes[0]->material;
      mesh->material.name += std::to_string(meshIndex);
      mesh->material.diffuseMap = mesh->material.name + ".jpg";
      //mesh->material.diffuseMapImage = it->second->meshes[0]->material.diffuseMapImage;

      // std::cout << "Info assigning has been finished" << std::endl;

      meshIndex++;

      if (mesh->hasUVs) {
        // std::cout << "Can split" << std::endl;
        // std::cout << "Calculating BBOX has been started" << std::endl;

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

        // std::cout << "Calculating BBOX has been finished" << std::endl;


        // std::cout << "Texture copying has been started" << std::endl;
        // Image &diffuse = mesh->material.diffuseMapImage;
        Image diffuse;

        diffuse.channels = 3;
        diffuse.width = textureWidth;
        diffuse.height = textureHeight;
        diffuse.data = new unsigned char[textureWidth * textureHeight * 3];

        // std::cout << "Texture copying has been finished" << std::endl;
        
        // std::cout << "Texture clipping has been started" << std::endl;
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

        // std::cout << "Texture clipping has been finished" << std::endl;

        mesh->material.diffuseMapImage = diffuse;
      } else {
        // std::cout << "Can not be split" << std::endl;
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

  meshMaterialMap.clear();

  // baseObject->traverse([](MeshObject mesh) {
  //   mesh->free();
  // });

  return resultGroup;
};

bool splitter::splitObject(GroupObject baseObject, GroupCallback fn, GroupCallback lodFn, bool isVertical = false) {
  unsigned int polygonCount = 0;

  baseObject->traverse([&](MeshObject mesh){
    polygonCount += mesh->faces.size();
  });

  //std::cout << "Total polygons: " << polygonCount << std::endl;

  // fn(baseObject);

  if (polygonCount <= 20000) {
    GroupObject resultGroup = splitter::splitUV(baseObject);
    resultGroup->name = "Chunk_";
    //fn(splitter::splitUV(baseObject));
    fn(resultGroup);

    // resultGroup->name = "Lod_0_Chunk_";
    // lodFn(simplifier::modify(resultGroup, 0.5f));

    return false;
  } else {

    for (unsigned int i = 2; i < 7; i++) {// 5 Levels
      if (polygonCount <= 20000 * i) {
        GroupObject resultGroup = splitter::splitUV(baseObject);

        resultGroup->name = std::string("Lod_") + std::to_string(i - 2) + std::string("_Chunk_");
        lodFn(simplifier::modify(resultGroup, 0.5f));

        //return false;
      }
    }

    // baseObject->name = "Lod_0_Chunk_";
    //lodFn(simplifier::modify(splitter::splitUV(baseObject), 0.5f));//splitter::splitUV()
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
  bool isRight = false;

  baseObject->traverse([&](MeshObject mesh){
    MeshObject leftMesh = MeshObject(new Mesh());
    MeshObject rightMesh = MeshObject(new Mesh());

    for (Face &face : mesh->faces) // access by reference to avoid copying
    {
      pos1 = mesh->position[face.positionIndices[0]];
      pos2 = mesh->position[face.positionIndices[1]];
      pos3 = mesh->position[face.positionIndices[2]];

      if (isVertical) {
        isLeft = pos1.x <= xMedian || pos2.x <= xMedian || pos3.x <= xMedian;
        isRight = pos1.x >= xMedian || pos2.x >= xMedian || pos3.x >= xMedian;

        if (isLeft) {
          leftMesh->faces.push_back(face);
        }
        if (isRight) {
          rightMesh->faces.push_back(face);
        }
      } else {
        isLeft = pos1.z <= zMedian || pos2.z <= zMedian || pos3.z <= zMedian;
        isRight = pos1.z >= zMedian || pos2.z >= zMedian || pos3.z >= zMedian;

        if (isLeft) {
          leftMesh->faces.push_back(face);
        }
        if (isRight) {
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
    splitter::straightLine(left, isVertical, true, xMedian, zMedian);
    splitter::splitObject(left, fn, lodFn, !isVertical);
  }

  if (right->meshes.size() != 0) {
    splitter::straightLine(right, isVertical, false, xMedian, zMedian);
    splitter::splitObject(right, fn, lodFn, !isVertical);
  }

  return true;
};

// TODO optimize a lot
void splitter::straightLineX(MeshObject &mesh, Face &face, bool isLeft, float xValue, std::vector<Vector3f> &position, std::vector<Vector3f> &normal, std::vector<Vector2f> &uv, std::vector<Face> &faces) {
  Vector3f pos1, pos2, pos3;

  pos1 = mesh->position[face.positionIndices[0]];
  pos2 = mesh->position[face.positionIndices[1]];
  pos3 = mesh->position[face.positionIndices[2]];

  int intersectionCount = 0;
  
  if (isLeft) {
    intersectionCount = int(pos1.x <= xValue) + int(pos2.x <= xValue) + int(pos3.x <= xValue);
  } else {
    intersectionCount = int(pos1.x >= xValue) + int(pos2.x >= xValue) + int(pos3.x >= xValue);
  }

  if (intersectionCount != 3) {
    bool aInside = true;
    bool bInside = true;
    bool cInside = true;

    if (isLeft) {
      if (pos1.x > xValue) aInside = false;
      if (pos2.x > xValue) bInside = false;
      if (pos3.x > xValue) cInside = false;
    } else {
      if (pos1.x < xValue) aInside = false;
      if (pos2.x < xValue) bInside = false;
      if (pos3.x < xValue) cInside = false;
    }

    if (intersectionCount == 1) {
      if (aInside) {
        mesh->position[face.positionIndices[1]].lerpToX(mesh->position[face.positionIndices[1]], mesh->position[face.positionIndices[0]], xValue);
        mesh->position[face.positionIndices[2]].lerpToX(mesh->position[face.positionIndices[2]], mesh->position[face.positionIndices[0]], xValue);
      } else if (bInside) {
        mesh->position[face.positionIndices[0]].lerpToX(mesh->position[face.positionIndices[0]], mesh->position[face.positionIndices[1]], xValue);
        mesh->position[face.positionIndices[2]].lerpToX(mesh->position[face.positionIndices[2]], mesh->position[face.positionIndices[1]], xValue);
      } else if (cInside) {
        mesh->position[face.positionIndices[0]].lerpToX(mesh->position[face.positionIndices[0]], mesh->position[face.positionIndices[2]], xValue);
        mesh->position[face.positionIndices[1]].lerpToX(mesh->position[face.positionIndices[1]], mesh->position[face.positionIndices[2]], xValue);
      }
    } else if (intersectionCount == 2) {
      float deltaA = 0.0f;
      float deltaB = 0.0f;

      Vector3f pos4;
      Face nextFace;

      if (aInside && bInside) {
        //std::cout << "Construct new vertex from C" << std::endl;

        deltaA = Vector3f::deltaX(pos3, pos1, xValue);
        deltaB = Vector3f::deltaX(pos3, pos2, xValue);

        pos4.lerp(pos3, pos2, deltaB);
        mesh->position[face.positionIndices[2]].lerp(pos3, pos1, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[1];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[2];

        nextFace.normalIndices[0] = face.normalIndices[1];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[2];

        nextFace.uvIndices[0] = face.uvIndices[1];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[2];

        faces.push_back(nextFace);
      } else if (aInside && cInside) {
        //std::cout << "Construct new vertex from B" << std::endl;

        deltaA = Vector3f::deltaX(pos2, pos3, xValue);
        deltaB = Vector3f::deltaX(pos2, pos1, xValue);

        pos4.lerp(pos2, pos1, deltaB);
        mesh->position[face.positionIndices[1]].lerp(pos2, pos3, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[0];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[1];

        nextFace.normalIndices[0] = face.normalIndices[0];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[1];

        nextFace.uvIndices[0] = face.uvIndices[0];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[1];

        faces.push_back(nextFace);
      } else if (bInside && cInside) {
        //std::cout << "Construct new vertex from B" << std::endl;

        deltaA = Vector3f::deltaX(pos1, pos3, xValue);
        deltaB = Vector3f::deltaX(pos1, pos2, xValue);

        pos4.lerp(pos1, pos2, deltaB);
        mesh->position[face.positionIndices[0]].lerp(pos1, pos3, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[1];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[0];

        nextFace.normalIndices[0] = face.normalIndices[1];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[0];

        nextFace.uvIndices[0] = face.uvIndices[1];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[0];

        faces.push_back(nextFace);
      }
    }
  }
};

// TODO optimize a lot
void splitter::straightLineZ(MeshObject &mesh, Face &face, bool isLeft, float zValue, std::vector<Vector3f> &position, std::vector<Vector3f> &normal, std::vector<Vector2f> &uv, std::vector<Face> &faces) {
  Vector3f pos1, pos2, pos3;

  pos1 = mesh->position[face.positionIndices[0]];
  pos2 = mesh->position[face.positionIndices[1]];
  pos3 = mesh->position[face.positionIndices[2]];

  int intersectionCount = 0;
  
  if (isLeft) {
    intersectionCount = int(pos1.z <= zValue) + int(pos2.z <= zValue) + int(pos3.z <= zValue);
  } else {
    intersectionCount = int(pos1.z >= zValue) + int(pos2.z >= zValue) + int(pos3.z >= zValue);
  }

  if (intersectionCount != 3) {
    bool aInside = true;
    bool bInside = true;
    bool cInside = true;

    if (isLeft) {
      if (pos1.z > zValue) aInside = false;
      if (pos2.z > zValue) bInside = false;
      if (pos3.z > zValue) cInside = false;
    } else {
      if (pos1.z < zValue) aInside = false;
      if (pos2.z < zValue) bInside = false;
      if (pos3.z < zValue) cInside = false;
    }

    if (intersectionCount == 1) {
      if (aInside) {
        mesh->position[face.positionIndices[1]].lerpToZ(mesh->position[face.positionIndices[1]], mesh->position[face.positionIndices[0]], zValue);
        mesh->position[face.positionIndices[2]].lerpToZ(mesh->position[face.positionIndices[2]], mesh->position[face.positionIndices[0]], zValue);
      } else if (bInside) {
        mesh->position[face.positionIndices[0]].lerpToZ(mesh->position[face.positionIndices[0]], mesh->position[face.positionIndices[1]], zValue);
        mesh->position[face.positionIndices[2]].lerpToZ(mesh->position[face.positionIndices[2]], mesh->position[face.positionIndices[1]], zValue);
      } else if (cInside) {
        mesh->position[face.positionIndices[0]].lerpToZ(mesh->position[face.positionIndices[0]], mesh->position[face.positionIndices[2]], zValue);
        mesh->position[face.positionIndices[1]].lerpToZ(mesh->position[face.positionIndices[1]], mesh->position[face.positionIndices[2]], zValue);
      }
    } else if (intersectionCount == 2) {
      float deltaA = 0.0f;
      float deltaB = 0.0f;

      Vector3f pos4;
      Face nextFace;

      if (aInside && bInside) {
        //std::cout << "Construct new vertex from C" << std::endl;

        deltaA = Vector3f::deltaZ(pos3, pos1, zValue);
        deltaB = Vector3f::deltaZ(pos3, pos2, zValue);

        pos4.lerp(pos3, pos2, deltaB);
        mesh->position[face.positionIndices[2]].lerp(pos3, pos1, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[1];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[2];

        nextFace.normalIndices[0] = face.normalIndices[1];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[2];

        nextFace.uvIndices[0] = face.uvIndices[1];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[2];

        faces.push_back(nextFace);
      } else if (aInside && cInside) {
        //std::cout << "Construct new vertex from B" << std::endl;

        deltaA = Vector3f::deltaZ(pos2, pos3, zValue);
        deltaB = Vector3f::deltaZ(pos2, pos1, zValue);

        pos4.lerp(pos2, pos1, deltaB);
        mesh->position[face.positionIndices[1]].lerp(pos2, pos3, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[0];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[1];

        nextFace.normalIndices[0] = face.normalIndices[0];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[1];

        nextFace.uvIndices[0] = face.uvIndices[0];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[1];

        faces.push_back(nextFace);
      } else if (bInside && cInside) {
        //std::cout << "Construct new vertex from B" << std::endl;

        deltaA = Vector3f::deltaZ(pos1, pos3, zValue);
        deltaB = Vector3f::deltaZ(pos1, pos2, zValue);

        pos4.lerp(pos1, pos2, deltaB);
        mesh->position[face.positionIndices[0]].lerp(pos1, pos3, deltaA);

        position.push_back(pos4);
        if (mesh->hasNormals) {
          normal.push_back(mesh->normal[face.normalIndices[2]]);
        }
        if (mesh->hasUVs) {
          uv.push_back(mesh->uv[face.uvIndices[2]]);
        }

        nextFace.positionIndices[0] = face.positionIndices[1];
        nextFace.positionIndices[1] = mesh->position.size() + position.size() - 1;
        nextFace.positionIndices[2] = face.positionIndices[0];

        nextFace.normalIndices[0] = face.normalIndices[1];
        nextFace.normalIndices[1] = mesh->normal.size() + normal.size() - 1;
        nextFace.normalIndices[2] = face.normalIndices[0];

        nextFace.uvIndices[0] = face.uvIndices[1];
        nextFace.uvIndices[1] = mesh->uv.size() + uv.size() - 1;
        nextFace.uvIndices[2] = face.uvIndices[0];

        faces.push_back(nextFace);
      }
    }
  }
};

void splitter::straightLine(GroupObject &baseObject, bool isVertical, bool isLeft, float xValue, float zValue) {
  std::vector<Vector3f> position;
  std::vector<Vector3f> normal;
  std::vector<Vector2f> uv;

  std::vector<Face> faces;

  baseObject->traverse([&](MeshObject mesh){
    position.clear();
    normal.clear();
    uv.clear();
    faces.clear();

    for (Face &face : mesh->faces) // access by reference to avoid copying
    {
      if (isVertical) {
        splitter::straightLineX(mesh, face, isLeft, xValue, position, normal, uv, faces);
      } else {
        splitter::straightLineZ(mesh, face, isLeft, zValue, position, normal, uv, faces);
      }
    }

    if (position.size() != 0) {
      std::copy(position.begin(), position.end(), std::back_inserter(mesh->position));
      std::copy(normal.begin(), normal.end(), std::back_inserter(mesh->normal));
      std::copy(uv.begin(), uv.end(), std::back_inserter(mesh->uv));
      std::copy(faces.begin(), faces.end(), std::back_inserter(mesh->faces));

      mesh->finish();
    }
  });
};

void splitter::splitObject(GroupObject baseObject, GroupCallback fn, GroupCallback lodFn) {
  splitter::splitObject(baseObject, fn, lodFn, true);
  //fn(splitter::splitUV(group));
};