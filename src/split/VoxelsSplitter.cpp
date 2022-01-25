#include "VoxelsSplitter.h"



VoxelFaceVertex& VoxelFaceTriangle::operator[] (size_t i)
{
  switch (i) {
    case 0: return this->a;
    case 1: return this->b;
    case 2: return this->c;
    default: throw "Index out of range";
  }
};

void Voxel::computeError() {
  this->geometricError = 0.0f;
  if (this->faces.size() > 0) {
    for (VoxelFaceTriangle &triangle : this->resultTriangles) {
      float triangleError = 0.0f;
      for(unsigned int i = 0; i < 3; i++) {

        glm::vec3 dist(0.0f);
        for (VoxelFacePtr &face : this->faces) {
          Vec3Result trPoint = math::clothestTrianglePoint(
            triangle[i].position.toGLM(), 
            face->vertices[0].position.toGLM(),
            face->vertices[1].position.toGLM(),
            face->vertices[2].position.toGLM()
          );

          if (trPoint.hasData) {
            dist += trPoint.data - triangle[i].position.toGLM();
            //triangleError += glm::length(trPoint.data - triangle[i].position.toGLM());
          }
        }

        
        triangleError += glm::length(dist) / std::max(1.0f, (float) this->faces.size());
      }

      this->geometricError += triangleError / 3.0f;
    }

    if (this->resultTriangles.size() > 0) {
      this->geometricError /= (float) this->resultTriangles.size();
    }
  }
};

Voxel::Voxel(glm::ivec3 position, glm::vec3 units, glm::vec3 offset) {
  this->position = position;
  this->units = units;

  this->voxelVertices[0] = glm::vec3(this->position.x,     this->position.y,     this->position.z)     * this->units;
  this->voxelVertices[1] = glm::vec3(this->position.x + 1, this->position.y,     this->position.z)     * this->units;
  this->voxelVertices[2] = glm::vec3(this->position.x + 1, this->position.y + 1, this->position.z)     * this->units;
  this->voxelVertices[3] = glm::vec3(this->position.x,     this->position.y + 1, this->position.z)     * this->units;
  this->voxelVertices[4] = glm::vec3(this->position.x,     this->position.y,     this->position.z + 1) * this->units;
  this->voxelVertices[5] = glm::vec3(this->position.x + 1, this->position.y,     this->position.z + 1) * this->units;
  this->voxelVertices[6] = glm::vec3(this->position.x + 1, this->position.y + 1, this->position.z + 1) * this->units;
  this->voxelVertices[7] = glm::vec3(this->position.x,     this->position.y + 1, this->position.z + 1) * this->units;

  for (unsigned int i = 0; i < 8; i++) {
    this->voxelVertices[i] = glm::vec3((float) this->voxelVertices[i].x, (float) this->voxelVertices[i].y, (float) this->voxelVertices[i].z);
    this->voxelVertices[i] += offset;
  }
};

bool Voxel::has(VoxelFacePtr &face) {
  std::vector<VoxelFacePtr>::iterator it = std::find(std::begin(this->faces), std::end(this->faces), face);

  return (it != this->faces.end());
};

Voxel::~Voxel() {
  this->faces.clear();
  this->resultTriangles.clear();
};

glm::vec2 Voxel::getClosestUV(glm::vec3 p) {
  glm::vec2 result;
  float dist = 999999.0f;

  for (VoxelFacePtr &facePtr : this->faces) {
    for (unsigned int i = 0; i < 3; i++) {
      VoxelFaceVertex &voxelFaceVertex = facePtr->vertices[i];

      float dt = voxelFaceVertex.position.distanceTo(Vector3f::fromGLM(p));
      if (dt <= dist) {
        dist = dt;
        result = voxelFaceVertex.uv.toGLM();
      }
    }
  }

  return result;
};


const unsigned int VoxelGrid::edgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

const int VoxelGrid::triTable[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

bool VoxelGrid::isFirst(unsigned int x, unsigned int y, unsigned int z, glm::ivec3 n) {
  int i = 1;
  glm::ivec3 pos = glm::ivec3(x, y, z) + (n * i);

  while (!this->isOutOfGrid(pos.x, pos.y, pos.z)) {
    if (this->has(pos.x, pos.y, pos.z)) {
      return false;
    }

    i++;
    pos += (n * i);
  }

  return true;
};

bool VoxelGrid::isOutOfGrid(unsigned int x, unsigned int y, unsigned int z) {
  int dx = (int) x;
  int dy = (int) y;
  int dz = (int) z;

  return (dx < 0 || dy < 0 || dz < 0) || (dx >= this->gridResolution.x || dy >= this->gridResolution.y || dz >= this->gridResolution.z);
};

bool VoxelGrid::hasNeighbor(unsigned int x, unsigned int y, unsigned int z, unsigned int index) {
  switch (index)
  {
    case 0:
      return this->has(x, y, z);
    case 1:
      return this->has(x + 1, y, z);
    case 2:
      return this->has(x + 1, y + 1, z);
    case 3:
      return this->has(x, y + 1, z);
    case 4:
      return this->has(x, y, z + 1);
    case 5:
      return this->has(x + 1, y, z + 1);
    case 6:
      return this->has(x + 1, y + 1, z + 1);
    case 7:
      return this->has(x, y + 1, z + 1);
  }

  return false;
};

VoxelPtr VoxelGrid::getNeighbor(unsigned int x, unsigned int y, unsigned int z, unsigned int index) {
  switch (index)
  {
    case 0:
      return this->get(x, y, z);
    case 1:
      return this->get(x + 1, y, z);
    case 2:
      return this->get(x + 1, y + 1, z);
    case 3:
      return this->get(x, y + 1, z);
    case 4:
      return this->get(x, y, z + 1);
    case 5:
      return this->get(x + 1, y, z + 1);
    case 6:
      return this->get(x + 1, y + 1, z + 1);
    case 7:
      return this->get(x, y + 1, z + 1);
  }

  return this->get(x, y, z);
};

void VoxelGrid::getClosestUV(VoxelFaceTriangle &triangle, glm::ivec3 voxelPos) {
  glm::vec3 triCenter = triangle.a.position.toGLM() + triangle.b.position.toGLM() + triangle.c.position.toGLM();
  triCenter /= 3.0f;

  VoxelFacePtr resFacePtr = nullptr;
  float dist = 999999.0f;

  int maxStep = 1;
  int minStep = -maxStep;

  for (int i = minStep; i <= maxStep; i++) {
    for (int k = minStep; k <= maxStep; k++) {
      for (int m = minStep; m <= maxStep; m++) {
        if (this->has(voxelPos.x + i, voxelPos.y + k, voxelPos.z + m)) {
          VoxelPtr voxel = this->get(voxelPos.x + i, voxelPos.y + k, voxelPos.z + m);
          for (VoxelFacePtr &facePtr : voxel->faces) {
            glm::vec3 sampleTriCenter = facePtr->vertices[0].position.toGLM() + facePtr->vertices[1].position.toGLM() + facePtr->vertices[2].position.toGLM();
            sampleTriCenter /= 3.0f;
            
            float dt = Vector3f::fromGLM(sampleTriCenter).distanceTo(Vector3f::fromGLM(triCenter));
            if (dt <= dist) {
              dist = dt;
              resFacePtr = facePtr;
            }
          }
        }
      }
    }
  }

  if (resFacePtr != nullptr) {
    triangle.a.uv = resFacePtr->vertices[0].uv;
    triangle.b.uv = resFacePtr->vertices[1].uv;
    triangle.c.uv = resFacePtr->vertices[2].uv;
  }
};

glm::vec2 VoxelGrid::getClosestUV(glm::ivec3 voxelPos, glm::vec3 pos) {
  glm::vec2 result;
  float dist = 999999.0f;

  Vector3f tpos = Vector3f::fromGLM(pos);

  int maxStep = 1;
  int minStep = -maxStep;

  for (int i = minStep; i <= maxStep; i++) {
    for (int k = minStep; k <= maxStep; k++) {
      for (int m = minStep; m <= maxStep; m++) {
        if (this->has(voxelPos.x + i, voxelPos.y + k, voxelPos.z + m)) {
          VoxelPtr voxel = this->get(voxelPos.x + i, voxelPos.y + k, voxelPos.z + m);
          for (VoxelFacePtr &facePtr : voxel->faces) {
            // math::clothestTrianglePointOld

            Vec3Result vecRes = math::clothestTrianglePointOld(
              tpos.toGLM(),
              facePtr->vertices[0].position.toGLM(),
              facePtr->vertices[1].position.toGLM(),
              facePtr->vertices[2].position.toGLM()
            );

            if (vecRes.hasData) {
              float dt = Vector3f::fromGLM(vecRes.data).distanceTo(tpos);
              if (dt <= dist) {
                dist = dt;
                result = vecRes.uv;//facePtr->vertices[0].uv.toGLM();
                // result = voxelFaceVertex.uv.toGLM();
              }
            }
            
            /*
            for (unsigned int t = 0; t < 3; t++) {
              VoxelFaceVertex &voxelFaceVertex = facePtr->vertices[t];

              float dt = voxelFaceVertex.position.distanceTo(tpos);
              if (dt <= dist) {
                dist = dt;
                result = voxelFaceVertex.uv.toGLM();
              }
            }
            */
          }
        }
      }
    }
  }

  return result;
};

glm::vec3 VoxelGrid::getVoxelVertex(unsigned int x, unsigned int y, unsigned int z, unsigned int index) {
  return this->get(x, y, z)->voxelVertices[index];
};

glm::vec3 VoxelGrid::intLinear(glm::vec3 p1, glm::vec3 p2, float valp1, float valp2) {
  // if (std::abs(this->isoLevel - valp1) < this->isoDelta) {
  //   return p1;
  // }

  // if (std::abs(this->isoLevel - valp2) < this->isoDelta) {
  //   return p2;
  // }

  // if (std::abs(valp1 - valp2) < this->isoDelta) {
  //   return p1;
  // }

  // float mu = (this->isoLevel - valp1) / (valp2 - valp1);


  float mu = valp2 / (valp1 + valp2);
  // mu = 0.0f;

  return glm::vec3(
    (float) (p1.x + mu * (p2.x - p1.x)),
    (float) (p1.y + mu * (p2.y - p1.y)),
    (float) (p1.z + mu * (p2.z - p1.z))
  );
};

VoxelPtr VoxelGrid::get(unsigned int x, unsigned int y, unsigned int z) {
  return this->data[x][y][z];
};

bool VoxelGrid::hasTriangles(unsigned int x, unsigned int y, unsigned int z) {
  if (this->isOutOfGrid(x, y, z)) {
    return false;
  }

  VoxelPtr voxel = this->data[x][y][z];

  if (voxel == NULL) {
    return false;
  }

  return (voxel->resultTriangles.size() > 0);
};

bool VoxelGrid::has(unsigned int x, unsigned int y, unsigned int z) {
  if (this->isOutOfGrid(x, y, z)) {
    return false;
  }

  VoxelPtr voxel = this->data[x][y][z];

  if (voxel == NULL) {
    return false;
  }

  return (voxel->faces.size() > 0);
};

void VoxelGrid::set(VoxelPtr &ptr, unsigned int x, unsigned int y, unsigned int z) {
  this->data[x][y][z] = ptr;
};

glm::ivec3 VoxelGrid::vecToGrid(float x, float y, float z) {
  glm::ivec3 position(
    (unsigned int) std::floor((x - this->gridOffset.x) / this->units.x),
    (unsigned int) std::floor((y - this->gridOffset.y) / this->units.y),
    (unsigned int) std::floor((z - this->gridOffset.z) / this->units.z)
  );

  position.x = std::min(this->gridResolution.x - 1, std::max(0, position.x));
  position.y = std::min(this->gridResolution.y - 1, std::max(0, position.y));
  position.z = std::min(this->gridResolution.z - 1, std::max(0, position.z));

  return position;
};

glm::vec3 VoxelGrid::gridToVec(unsigned int x, unsigned int y, unsigned int z) {
  return glm::vec3(
    x * this->units.x + this->gridOffset.x,
    y * this->units.y + this->gridOffset.y,
    z * this->units.z + this->gridOffset.z
  );
};


bool VoxelGrid::firstOfX(int cellX, int cellY, int cellZ) {
  if (cellX == 0) {
    return true;
  }

  for (int dx = cellX - 1; dx >= 0; dx--) {
    if (this->has(dx, cellY, cellZ)) {
      return false;
    }
  }

  return true;
};

bool VoxelGrid::firstOfZ(int cellX, int cellY, int cellZ) {
  if (cellZ == 0) {
    return true;
  }

  for (int dz = cellZ - 1; dz >= 0; dz--) {
    if (this->has(cellX, cellY, dz)) {
      return false;
    }
  }

  return true;
};

bool VoxelGrid::lastOfX(int cellX, int cellY, int cellZ) {
  if (cellX == this->gridResolution.x) {
    return true;
  }

  for (int dx = cellX + 1; dx <= this->gridResolution.x; dx++) {
    if (this->has(dx, cellY, cellZ)) {
      return false;
    }
  }
  
  return true;
};

bool VoxelGrid::lastOfZ(int cellX, int cellY, int cellZ) {
  if (cellZ == this->gridResolution.z) {
    return true;
  }

  for (int dz = cellZ + 1; dz <= this->gridResolution.z; dz++) {
    if (this->has(cellX, cellY, dz)) {
      return false;
    }
  }
  
  return true;
};


float VoxelGrid::getIntValue(unsigned int x, unsigned int y, unsigned int z) {
  unsigned int faces = std::min((unsigned int) 32, (unsigned int) this->get(x, y, z)->faces.size());

  return 0.5f;

  // return ((float) faces) / 32.0f;
};

float VoxelGrid::getIntValue(unsigned int x, unsigned int y, unsigned int z, unsigned int index) {
  /*
  if (this->firstOfX(x, y, z) || this->firstOfZ(x, y, z)) {
    return 1.0;
  }

  if (this->lastOfX(x, y, z) || this->lastOfZ(x, y, z)) {
    return 1.0;
  }
  */
  return 0.5f;

  //return (float) y / float (this->gridResolution.y);
  // VoxelPtr n = this->hasNeighbor(x, y, z, index) ? this->getNeighbor(x, y, z, index) : this->getNeighbor(x, y, z, 0);

  // unsigned int faces = (unsigned int) n->faces.size();

  // return ((float) faces) / this->facesBox.y;
};

VoxelFaceQuad VoxelGrid::getQuad(
  Vector3f a, Vector3f b, Vector3f c, Vector3f d,
  Vector3f n1, Vector3f n2,
  Vector2f t1, Vector2f t2, Vector2f t3, Vector2f t4
) {
  VoxelFaceQuad quad;

  // Triangle 1
  quad.t1.a.position = a;
  quad.t1.a.normal = n1;
  quad.t1.a.uv = t1;

  quad.t1.b.position = b;
  quad.t1.b.normal = n1;
  quad.t1.b.uv = t2;

  quad.t1.c.position = d;
  quad.t1.c.normal = n1;
  quad.t1.c.uv = t4;

  quad.t1.normal = n1;

  // Triangle 2
  quad.t2.a.position = b;
  quad.t2.a.normal = n2;
  quad.t2.a.uv = t2;

  quad.t2.b.position = c;
  quad.t2.b.normal = n2;
  quad.t2.b.uv = t3;

  quad.t2.c.position = d;
  quad.t2.c.normal = n2;
  quad.t2.c.uv = t4;

  quad.t2.normal = n2;

  return quad;
};

std::vector<VoxelFaceTriangle> VoxelGrid::getVertices(unsigned int x, unsigned int y, unsigned int z) {
  std::vector<VoxelFaceTriangle> result;

  /*
   * Determine the index into the edge table which
   * tells us which vertices are inside of the surface
   */
  unsigned int cubeindex = 0;
  if (this->hasNeighbor(x, y, z, 0)) cubeindex |= 1;
  if (this->hasNeighbor(x, y, z, 1)) cubeindex |= 2;
  if (this->hasNeighbor(x, y, z, 2)) cubeindex |= 4;
  if (this->hasNeighbor(x, y, z, 3)) cubeindex |= 8;
  if (this->hasNeighbor(x, y, z, 4)) cubeindex |= 16;
  if (this->hasNeighbor(x, y, z, 5)) cubeindex |= 32;
  if (this->hasNeighbor(x, y, z, 6)) cubeindex |= 64;
  if (this->hasNeighbor(x, y, z, 7)) cubeindex |= 128;

  /* Cube is entirely in/out of the surface */
  if (this->edgeTable[cubeindex] == 0) {
    return result;
  }
  
  // Do not need bottom 
  // if (this->isFirst(x, y, z, glm::ivec3(0, -1, 0))) {
  //   return result;
  // }

  glm::vec3 vertices[12];
  for (int i = 0; i < 12; i++) {
    vertices[i] = glm::vec3(0.0f);
  }

  VoxelPtr voxel = this->get(x, y, z);
  glm::ivec3 voxelPos(x, y, z);

  /* Find the vertices where the surface intersects the cube */
  if (this->edgeTable[cubeindex] & 1) {
    vertices[0] = this->intLinear(voxel->voxelVertices[0], voxel->voxelVertices[1], this->getIntValue(x, y, z, 0), this->getIntValue(x, y, z, 1));
  }
  if (this->edgeTable[cubeindex] & 2) {
    vertices[1] = this->intLinear(voxel->voxelVertices[1], voxel->voxelVertices[2], this->getIntValue(x, y, z, 1), this->getIntValue(x, y, z, 2));
  }
  if (this->edgeTable[cubeindex] & 4) {
    vertices[2] = this->intLinear(voxel->voxelVertices[2], voxel->voxelVertices[3], this->getIntValue(x, y, z, 2), this->getIntValue(x, y, z, 3));
  }
  if (this->edgeTable[cubeindex] & 8) {
    vertices[3] = this->intLinear(voxel->voxelVertices[3], voxel->voxelVertices[0], this->getIntValue(x, y, z, 3), this->getIntValue(x, y, z, 0));
  }
  if (this->edgeTable[cubeindex] & 16) {
    vertices[4] = this->intLinear(voxel->voxelVertices[4], voxel->voxelVertices[5], this->getIntValue(x, y, z, 4), this->getIntValue(x, y, z, 5));
  }
  if (this->edgeTable[cubeindex] & 32) {
    vertices[5] = this->intLinear(voxel->voxelVertices[5], voxel->voxelVertices[6], this->getIntValue(x, y, z, 5), this->getIntValue(x, y, z, 6));
  }
  if (this->edgeTable[cubeindex] & 64) {
    vertices[6] = this->intLinear(voxel->voxelVertices[6], voxel->voxelVertices[7], this->getIntValue(x, y, z, 6), this->getIntValue(x, y, z, 7));
  }
  if (this->edgeTable[cubeindex] & 128) {
    vertices[7] = this->intLinear(voxel->voxelVertices[7], voxel->voxelVertices[4], this->getIntValue(x, y, z, 7), this->getIntValue(x, y, z, 4));
  }
  if (this->edgeTable[cubeindex] & 256) {
    vertices[8] = this->intLinear(voxel->voxelVertices[0], voxel->voxelVertices[4], this->getIntValue(x, y, z, 0), this->getIntValue(x, y, z, 4));
  }
  if (this->edgeTable[cubeindex] & 512) {
    vertices[9] = this->intLinear(voxel->voxelVertices[1], voxel->voxelVertices[5], this->getIntValue(x, y, z, 1), this->getIntValue(x, y, z, 5));
  }
  if (this->edgeTable[cubeindex] & 1024) {
    vertices[10] = this->intLinear(voxel->voxelVertices[2], voxel->voxelVertices[6], this->getIntValue(x, y, z, 2), this->getIntValue(x, y, z, 6));
  }
  if (this->edgeTable[cubeindex] & 2048) {
    vertices[11] = this->intLinear(voxel->voxelVertices[3], voxel->voxelVertices[7], this->getIntValue(x, y, z, 3), this->getIntValue(x, y, z, 7));
  }

  /* Create the triangle */
  //  unsigned int ntriang = 0;
  for (unsigned int i = 0; triTable[cubeindex][i] != -1; i += 3) {
    VoxelFaceTriangle triangle;

    glm::vec3 a = vertices[this->triTable[cubeindex][i    ]];
    glm::vec3 b = vertices[this->triTable[cubeindex][i + 1]];
    glm::vec3 c = vertices[this->triTable[cubeindex][i + 2]];

    glm::vec3 n = glm::cross(a - b, c - b);
    float length = glm::length(n);

    if (length == 0.0f && n.x == 0.0f) {
      // std::cout << "Length is zero " 
      // << "(" << a.x << ", " << a.y << ", " << a.z << ")"
      // << "(" << b.x << ", " << b.y << ", " << b.z << ")"
      // << "(" << c.x << ", " << c.y << ", " << c.z << ")"
      // << std::endl;
    }

    if (length == 0.0f) {
      n = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    triangle.normal = Vector3f::fromGLM(glm::normalize(n));
    // triangle.normal.x *= -1.0f;
    // triangle.normal.y *= -1.0f;
    // triangle.normal.z *= -1.0f;
    // Vector3f voxelAvgNormal = Vector3f::fromGLM(voxel->averageNormal);

    // if (isnan(triangle.normal.x)) {
    //   std::cout << "Is nan (" << triangle.normal.x << ")" << std::endl;
    // }

    //float angle = voxelAvgNormal.angleTo(triangle.normal);

    //if (true) {// voxelAvgNormal.angleLess90(triangle.normal)
      triangle.a.position = Vector3f::fromGLM(a);
      triangle.b.position = Vector3f::fromGLM(b);
      triangle.c.position = Vector3f::fromGLM(c);
    
      triangle.a.normal = triangle.normal;
      triangle.b.normal = triangle.normal;
      triangle.c.normal = triangle.normal;

      if (voxel->faces.size() > 0) {
        // triangle.a.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.a.position.toGLM()));
        // triangle.b.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.b.position.toGLM()));
        // triangle.c.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.c.position.toGLM()));
      }

      voxel->resultTriangles.push_back(triangle);
      result.push_back(triangle);
    //}
  }

  return result;
};

bool VoxelGrid::pointInCell(glm::vec3 p, glm::ivec3 cell) {
  // glm::vec3 t = this->gridToVec(cell.x, cell.y, cell.z);//(glm::vec3(cell.x, cell.y, cell.z) * this->units);

  // return (p.x >= t.x && p.x <= (t.x + this->units.x)) || (p.y >= t.y && p.y <= (t.y + this->units.y)) || (p.z >= t.z && p.z <= (t.z + this->units.z));

  glm::ivec3 t = this->vecToGrid(p.x, p.y, p.z);
  return (t.x == cell.x) && (t.y == cell.y) && (t.z == cell.z);
};

bool Voxel::intersects(glm::vec3 from, glm::vec3 to) {
  return true;
};

bool VoxelGrid::triangleIntersectsCell(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::ivec3 cell) {
  //glm::vec3 cellCenter = (glm::vec3(cell.x, cell.y, cell.z) * this->units) + (this->units * 0.5f);
  glm::ivec3 xp = this->vecToGrid(a.x, a.y, a.z);
  glm::ivec3 yp = this->vecToGrid(b.x, b.y, b.z);
  glm::ivec3 zp = this->vecToGrid(c.x, c.y, c.z);

  glm::ivec3 minValues(999, 999, 999);
  glm::ivec3 maxValues(0, 0, 0);

  minValues.x = std::min(xp.x, std::min(yp.x, zp.x));
  minValues.y = std::min(xp.y, std::min(yp.y, zp.y));
  minValues.z = std::min(xp.z, std::min(yp.z, zp.z));

  maxValues.x = std::max(xp.x, std::max(yp.x, zp.x));
  maxValues.y = std::max(xp.y, std::max(yp.y, zp.y));
  maxValues.z = std::max(xp.z, std::max(yp.z, zp.z));

  glm::vec3 boxCenter(0.0f, 0.0f, 0.0f);
  glm::vec3 halfSize = this->units * 0.5f;

  for (unsigned int dx = minValues.x; dx <= maxValues.x; dx++) {
    for (unsigned int dy = minValues.y; dy <= maxValues.y; dy++) {
      for (unsigned int dz = minValues.z; dz <= maxValues.z; dz++) {
        boxCenter = this->gridToVec(dx, dy, dz) + (this->units * 0.5f);

        if (TriangleBox::test(boxCenter, halfSize, a, b, c)) {
          return true;
        }
      }
    }
  }

  // if (this->pointInCell(a, cell)) {
  //   return true;
  // }

  // if (this->pointInCell(b, cell)) {
  //   return true;
  // }

  // if (this->pointInCell(c, cell)) {
  //   return true;
  // }

  return false;
};

void VoxelGrid::voxelize(MeshObject &mesh) {
  for (Face &face : mesh->faces) {
    VoxelFacePtr voxelFace = std::make_shared<VoxelFace>();
    for (unsigned int i = 0; i < 3; i++) {
      VoxelFaceVertex voxelFaceVertex;

      voxelFaceVertex.position = mesh->position[face.positionIndices[i]];

      if (mesh->hasNormals) {
        voxelFace->hasNormals = true;
        voxelFaceVertex.normal = mesh->normal[face.normalIndices[i]];
      }

      if (mesh->hasUVs) {
        voxelFace->hasUVs = true;
        voxelFaceVertex.uv = mesh->uv[face.uvIndices[i]];
      }

      voxelFace->vertices[i] = voxelFaceVertex;
    }

    voxelFace->materialName = mesh->material->name;

    glm::vec3 a = voxelFace->vertices[0].normal.toGLM();
    glm::vec3 b = voxelFace->vertices[1].normal.toGLM();
    glm::vec3 c = voxelFace->vertices[2].normal.toGLM();

    glm::vec3 normal = glm::normalize(glm::cross(c - a, b - a));


    /** Get Min/Max cells of the target triangle */
    glm::ivec3 minGridCell(999, 999, 999);
    glm::ivec3 maxGridCell(-999, -999, -999);

    glm::ivec3 cellA = this->vecToGrid(voxelFace->vertices[0].position.x, voxelFace->vertices[0].position.y, voxelFace->vertices[0].position.z);
    glm::ivec3 cellB = this->vecToGrid(voxelFace->vertices[1].position.x, voxelFace->vertices[1].position.y, voxelFace->vertices[1].position.z);
    glm::ivec3 cellC = this->vecToGrid(voxelFace->vertices[2].position.x, voxelFace->vertices[2].position.y, voxelFace->vertices[2].position.z);

    minGridCell.x = std::min(std::min(cellA.x, cellB.x), cellC.x);
    minGridCell.y = std::min(std::min(cellA.y, cellB.y), cellC.y);
    minGridCell.z = std::min(std::min(cellA.z, cellB.z), cellC.z);

    maxGridCell.x = std::max(std::max(cellA.x, cellB.x), cellC.x);
    maxGridCell.y = std::max(std::max(cellA.y, cellB.y), cellC.y);
    maxGridCell.z = std::max(std::max(cellA.z, cellB.z), cellC.z);

    // Add face to all cells that intersects with it
    for (int dx = minGridCell.x; dx <= maxGridCell.x; dx++) {
      for (int dy = minGridCell.y; dy <= maxGridCell.y; dy++) { 
        for (int dz = minGridCell.z; dz <= maxGridCell.z; dz++) {
          bool intersects = this->triangleIntersectsCell(
            voxelFace->vertices[0].position.toGLM(),
            voxelFace->vertices[1].position.toGLM(),
            voxelFace->vertices[2].position.toGLM(),
            glm::ivec3(dx, dy, dz)
          );

          if (intersects) {
            VoxelPtr ptr = this->get(dx, dy, dz);

            if (ptr->faces.size() == 0) {
              ptr->averageNormal = normal;
            } else {
              ptr->averageNormal += normal;
              ptr->averageNormal = glm::normalize(ptr->averageNormal * 0.5f);
            }
            
            ptr->faces.push_back(voxelFace);
          }
        }
      }
    }

    /*
    for (unsigned int k = 0; k < 3; k++) {
      VoxelFaceVertex v = voxelFace->vertices[k];
      glm::ivec3 cell = this->vecToGrid(v.position.x, v.position.y, v.position.z);

      VoxelPtr ptr = this->get(cell.x, cell.y, cell.z);

      if (ptr->faces.size() == 0) {
        ptr->averageNormal = normal;
      } else {
        ptr->averageNormal += normal;
        ptr->averageNormal = glm::normalize(ptr->averageNormal * 0.5f);
      }
      
      ptr->faces.push_back(voxelFace);
    }
    */
  }
};

void VoxelGrid::rasterize(GroupObject &src, GroupObject &dest) {
  // std::cout << "Init grid" << std::endl;
  std::cout << "Rasterize has been started" << std::endl;
  src->computeBoundingBox();
  BBoxf dimensionsBox = src->boundingBox.clone();

  glm::vec3 dimSize = dimensionsBox.getSize();

  float maxUnit = std::max(
    dimSize.x / (float) this->gridResolution.x,
    std::max(
      dimSize.y / (float) this->gridResolution.y,
      dimSize.z / (float) this->gridResolution.z
    )
  );

  this->gridOffset = glm::vec3(dimensionsBox.min.x, dimensionsBox.min.y, dimensionsBox.min.z);
  this->units = glm::vec3(maxUnit, maxUnit, maxUnit);// Temporary (probably)
  //this->init();

  this->clear();

  std::map<std::string, MeshObject> materialMeshMap;

  // unsigned int meshIndex = 0;
  src->traverse([&](MeshObject target){
    std::cout << "Voxelize has been started" << std::endl;
    this->voxelize(target);
    std::cout << "Voxelize has been finished" << std::endl;

    MeshObject mesh = MeshObject(new Mesh());
    // mesh->name = std::string("Voxels_") + target->name + std::string("_") + std::to_string(meshIndex);
    // mesh->material.name = "VoxelMeshMaterial";
    // mesh->material.color.set(0.5f, 0.5f, 0.5f);

    mesh->name = target->name;
    //mesh->material = target->material;//->clone(true);
    mesh->material = target->material;//->clone(true);
    //mesh->material->diffuseMapImage = target->material->diffuseMapImage;

    // mesh->material->name = target->material.name;
    // mesh->material->color.set(0.5f, 1.0f, 0.0f);

    mesh->hasNormals = target->hasNormals;
    mesh->hasUVs = target->hasUVs;

    if (materialMeshMap.count(mesh->material->name) == 0) {
      materialMeshMap[mesh->material->name] = mesh;
    }

    // std::cout << "Rasterize" << std::endl;
    for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
      for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
        for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
          this->getVertices(x, y, z);
        }
      }
    }

    float geometricError = 0.0f;
    unsigned int voxelsUsed = 0;

    for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
      for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
        for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
          VoxelPtr voxel = this->get(x, y, z);
          voxel->computeError();
          geometricError += voxel->geometricError;
          if (voxel->geometricError != 0.0f) {
            voxelsUsed++;
          }

          glm::ivec3 voxelPos(x, y, z);

          for (VoxelFaceTriangle &triangle : voxel->resultTriangles) {
            this->getClosestUV(triangle, voxelPos);
            /*
            triangle.a.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.a.position.toGLM()));
            triangle.b.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.b.position.toGLM()));
            triangle.c.uv = Vector2f::fromGLM(this->getClosestUV(voxelPos, triangle.c.position.toGLM()));
            */
          }
        }
      }
    }

    if (voxelsUsed > 0) {
      geometricError /= (float) voxelsUsed;
    }

    // std::cout << "Finish mesh" << std::endl;
    this->build(mesh, materialMeshMap);
    mesh->computeUVBox();
    mesh->computeBoundingBox();
    mesh->finish();

    // MeshObject simplified = simplifier::modify(mesh, 0.1f);

    mesh->triangulate();
    mesh->geometricError = geometricError;
    
    dest->meshes.push_back(mesh);
    // meshIndex++;
    this->clear();
    // mesh->free(false);
  });

  std::cout << "Rasterize has been finished" << std::endl;

  // dest->computeGeometricError();

  /*
  this->build(dest->meshes[0], materialMeshMap);
  dest->meshes[0]->finish();
  */

  // dest->traverse([&](MeshObject target){
  //   this->build(target, materialMeshMap);
  //   target->finish();
  // });

  //this->clear();
};

void VoxelGrid::build(VoxelFaceTriangle &triangle, VoxelFaceVertex &vertex, std::vector<LinkedPosition> &list, unsigned int x, unsigned int y, unsigned int z) {
  VoxelFaceVertex resultVertex;
  resultVertex.position = vertex.position;
  resultVertex.normal = vertex.normal;
  resultVertex.uv = vertex.uv;

  int index = list.size();
  resultVertex.index = index;
  vertex.index = index;

  LinkedPosition linked;
  linked.linkedTriangles.push_back(triangle);
  linked.vertex = resultVertex;

  // Look for the same vertex in adjacent cells
  for (int i = -1; i <= 1; i++) {
    for (int k = -1; k <= 1; k++) {
      for (int m = -1; m <= 1; m++) {
        if (this->hasTriangles(x + i, y + k, z + m)) {
          VoxelPtr voxel = this->get(x + i, y + k, z + m);

          // Go for each triangle in target
          for (VoxelFaceTriangle &vTriangle : voxel->resultTriangles) {// Access by ref to modify origin
            // Go for each position and compare
            bool exists = false;

            // if (vTriangle.a.index == -1) {// Not calculated vertex A
              if (vertex.position.equals(vTriangle.a.position) && false) {
                vTriangle.a.index = index;
                exists = true;
              }
            // } else {
            //   // std::cout << "Vertex already assigned: (" << i << ", " << k << ", " << m << ")" << std::endl;
            // }
            
            // if (vTriangle.b.index == -1) {// Not calculated vertex B
              if (vertex.position.equals(vTriangle.b.position) && false) {
                vTriangle.b.index = index;
                exists = true;  
              }
            // } else {
            //   // std::cout << "Vertex already assigned: (" << i << ", " << k << ", " << m << ")" << std::endl;
            // }

            //if (vTriangle.c.index == -1) {// Not calculated vertex C
              if (vertex.position.equals(vTriangle.c.position) && false) {
                vTriangle.c.index = index;
                exists = true;
              }
            // } else {
            //  // std::cout << "Vertex already assigned: (" << i << ", " << k << ", " << m << ")" << std::endl;
            // }

            if (exists) {
              linked.linkedTriangles.push_back(vTriangle);
            }
          }

        }
      }
    }
  }

  list.push_back(linked);
};

void VoxelGrid::build(MeshObject &mesh, std::map<std::string, MeshObject> &materialMeshMap) {
  std::vector<LinkedPosition> linkedList;
  std::vector<VoxelFaceTriangle> triangles;

  for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
    for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
      for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
        VoxelPtr target = this->get(x, y, z);
        // Go for each triangle in target
        for (VoxelFaceTriangle &triangle : target->resultTriangles) {// Access by ref to modify origin
          // Go for each position, build and save into linked list
          if (triangle.a.index == -1) {// Not calculated
            this->build(triangle, triangle.a, linkedList, x, y, z);
          }
          
          if (triangle.b.index == -1) {// Not calculated
            this->build(triangle, triangle.b, linkedList, x, y, z);
          }

          if (triangle.c.index == -1) {// Not calculated
            this->build(triangle, triangle.c, linkedList, x, y, z);
          }
        }
      }
    }
  }

  for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
    for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
      for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
        //if (this->has(x, y, z)) {
          VoxelPtr target = this->get(x, y, z);
          // Go for each triangle in target
          for (VoxelFaceTriangle &triangle : target->resultTriangles) {// Access by ref to modify origin
            // Go for each position and save into linked list   
            triangles.push_back(triangle);
          }
        //}
      }
    }
  }
  
  //MeshObject targetMesh = mesh;//materialMeshMap.begin()->second;

  for (LinkedPosition &linked : linkedList) {
    mesh->position.push_back(linked.vertex.position);

    if (linked.linkedTriangles.size() > 0 && false) {
      glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);
      
      for (VoxelFaceTriangle &tr : linked.linkedTriangles) {
        normal += tr.normal.toGLM();
      }

      normal /= (float) linked.linkedTriangles.size();
      mesh->normal.push_back(Vector3f::fromGLM(glm::normalize(normal)));
    } else {
      mesh->normal.push_back(linked.vertex.normal);
    }
    // mesh->uv.push_back(linked.vertex.uv);

    //if (mesh->uv.size() > 0) {
      mesh->uv.push_back(linked.vertex.uv);
    //}
  }

  for (VoxelFaceTriangle &triangle : triangles) {
    Face face;

    face.positionIndices[0] = (unsigned int) triangle.a.index;
    face.positionIndices[1] = (unsigned int) triangle.b.index;
    face.positionIndices[2] = (unsigned int) triangle.c.index;

    face.normalIndices[0] = face.positionIndices[0];
    face.normalIndices[1] = face.positionIndices[1];
    face.normalIndices[2] = face.positionIndices[2];

    face.uvIndices[0] = face.positionIndices[0];
    face.uvIndices[1] = face.positionIndices[1];
    face.uvIndices[2] = face.positionIndices[2];

    mesh->faces.push_back(face);
  }
};

void VoxelGrid::init() {
  this->data = new VoxelPtr**[this->gridResolution.x];

  for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) { 
    this->data[x] = new VoxelPtr*[this->gridResolution.y];

    for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
      this->data[x][y] = new VoxelPtr[this->gridResolution.z];

      for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
        this->data[x][y][z] = std::make_shared<Voxel>(glm::ivec3(x, y, z), this->units, this->gridOffset);
      }
    }
  }
};

void VoxelGrid::clear() {
  for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
    for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
      for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
        this->data[x][y][z].reset(new Voxel(glm::ivec3(x, y, z), this->units, this->gridOffset));
        //this->data[x][y][z] = std::make_shared<Voxel>(glm::ivec3(x, y, z), this->units);
      }
    }
  }
};

void VoxelGrid::free() {
  // Delete each sublevel 
  for (unsigned int x = 0; x < (unsigned int) this->gridResolution.x; x++) {
    for (unsigned int y = 0; y < (unsigned int) this->gridResolution.y; y++) {
      for (unsigned int z = 0; z < (unsigned int) this->gridResolution.z; z++) {
        this->data[x][y][z].reset();
      }

      delete[] this->data[x][y];
    }

    delete[] this->data[x];
  }

  delete[] this->data;
};

VoxelsSplitter::VoxelsSplitter() {
  //this->grid.init();
};

bool VoxelsSplitter::split(GroupObject target) {
  this->IDGen.reset();

  return this->split(target, this->IDGen.id, 0, true);
};

GroupObject VoxelsSplitter::halfMesh(GroupObject target, bool divideVertical) {
  float xMedian = 0.0f;
  float zMedian = 0.0f;
  unsigned int verticesCount = 0;

  target->traverse([&](MeshObject mesh){
    for (Vector3f &position : mesh->position) // access by reference to avoid copying
    {
      if (divideVertical) {
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

  GroupObject base = GroupObject(new Group());
  GroupObject left = GroupObject(new Group());
  GroupObject right = GroupObject(new Group());


  Vector3f pos1, pos2, pos3;

  bool isLeft = false;
  bool isRight = false;

  target->traverse([&](MeshObject mesh){
    MeshObject leftMesh = MeshObject(new Mesh());
    MeshObject rightMesh = MeshObject(new Mesh());

    for (Face &face : mesh->faces) // access by reference to avoid copying
    {
      pos1 = mesh->position[face.positionIndices[0]];
      pos2 = mesh->position[face.positionIndices[1]];
      pos3 = mesh->position[face.positionIndices[2]];

      if (divideVertical) {
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
      leftMesh->finish();
      leftMesh->triangulate();

      left->meshes.push_back(leftMesh);
    }

    if (rightMesh->faces.size() != 0) {
      rightMesh->name = mesh->name;
      rightMesh->material = mesh->material;
      rightMesh->hasNormals = mesh->hasNormals;
      rightMesh->hasUVs = mesh->hasUVs;

      rightMesh->remesh(mesh->position, mesh->normal, mesh->uv);
      rightMesh->finish();
      rightMesh->triangulate();

      right->meshes.push_back(rightMesh);
    }
  });

  if (left->meshes.size() != 0) { 
    base->children.push_back(left);
  }

  if (right->meshes.size() != 0) {
    base->children.push_back(right);
  }
  
  return base;
};

GroupObject VoxelsSplitter::decimate(GroupObject target, GridRef grid) {
  GroupObject result = GroupObject(new Group());

  grid->rasterize(target, result);

  // result->computeUVBox();
  result->computeBoundingBox();

  return result;
};

/*
void VoxelsSplitter::createVoxel(MeshObject &mesh, glm::ivec3 cell) {
  // (this->gridResolution.z - 1)
  if ((cell.y == 0) || this->grid[cell.x][cell.y - 1][cell.z] == NULL) { // Has top
    this->createVoxelPlane(mesh, cell, glm::vec3(0.0f, -1.0f, 0.0f));
  }

  if ((cell.y == (this->gridResolution.y - 1)) || this->grid[cell.x][cell.y + 1][cell.z] == NULL) { // Has bottom
    this->createVoxelPlane(mesh, cell, glm::vec3(0.0f, 1.0f, 0.0f));
  }

  if ((cell.z > 0) && this->grid[cell.x][cell.y][cell.z - 1] == NULL) { // Has front
    this->createVoxelPlane(mesh, cell, glm::vec3(0.0f, 0.0f, -1.0f));
  }
  
  if ((cell.z < (this->gridResolution.z - 1)) && this->grid[cell.x][cell.y][cell.z + 1] == NULL) { // Has back
    this->createVoxelPlane(mesh, cell, glm::vec3(0.0f, 0.0f, 1.0f));
  }

  if ((cell.x > 0) && this->grid[cell.x - 1][cell.y][cell.z] == NULL) { // Has left
    this->createVoxelPlane(mesh, cell, glm::vec3(-1.0f, 0.0f, 0.0f));
  }

  if ((cell.x < (this->gridResolution.x - 1)) && this->grid[cell.x + 1][cell.y][cell.z] == NULL) { // Has right
    this->createVoxelPlane(mesh, cell, glm::vec3(1.0f, 0.0f, 0.0f));
  }
  
};

void VoxelsSplitter::createVoxelPlane(MeshObject &mesh, glm::ivec3 cell, glm::vec3 normal) {
  unsigned int lastVertexIndex = mesh->position.size();
  unsigned int lastNormalIndex = mesh->normal.size();

  Vector3f a0, a1, a2, a3;

  glm::vec3 p0(
    -0.5f * units.x,
    0.0f,
    -0.5f * units.z
  );

  glm::vec3 p1(
    0.5f * units.x,
    0.0f,
    -0.5f * units.z
  );

  glm::vec3 p2(
    0.5f * units.x,
    0.0f,
    0.5f * units.z
  );

  glm::vec3 p3(
    -0.5f * units.x,
    0.0f,
    0.5f * units.z
  );

  float angle = 0.0f;
  glm::vec3 axis(0.0f, 1.0f, 0.0f);
  glm::vec3 shift(0.5f * units.x, 0.0f, 0.5f * units.z);

  if (normal.x == normal.z && normal.x == 0.0f) { // Y is here
    // Top/Bottom quad
    angle = (1.0f - ((0.5 * normal.y) + 0.5)) * 3.141593;
    axis = glm::vec3(1.0f, 0.0f, 0.0f);
    shift = glm::vec3(0.0f, 0.5f * normal.y * units.y, 0.0f);
  } else if (normal.x == normal.y && normal.x == 0.0f) { // Z is here
    // Front/Back quad
    angle = 0.5f * normal.z * 3.141593;
    axis = glm::vec3(1.0f, 0.0f, 0.0f);
    shift = glm::vec3(0.0f, 0.0f, 0.5f * units.z * normal.z);
  } else if (normal.z == normal.y && normal.z == 0.0f) { // Z is here
    // Left/Right quad
    angle = 0.5f * normal.x * 3.141593;
    axis = glm::vec3(0.0f, 0.0f, 1.0f);
    shift = glm::vec3(0.5f * units.x * normal.x, 0.0f, 0.0f);
  }

  p0 = glm::rotate(p0, angle, axis);
  p1 = glm::rotate(p1, angle, axis);
  p2 = glm::rotate(p2, angle, axis);
  p3 = glm::rotate(p3, angle, axis);

  shift += glm::vec3(units.x * cell.x, units.y * cell.y, units.z * cell.z);

  p0 += shift;
  p1 += shift;
  p2 += shift;
  p3 += shift;

  Vector3f x00 = {p0.x, p0.y, p0.z};
  Vector3f x10 = {p1.x, p1.y, p1.z};
  Vector3f x11 = {p2.x, p2.y, p2.z};
  Vector3f x01 = {p3.x, p3.y, p3.z};

  Vector3f n = {normal.x, normal.y, normal.z} ;

  mesh->position.push_back(x00);
  mesh->position.push_back(x10);
  mesh->position.push_back(x11);
  mesh->position.push_back(x01);

  mesh->normal.push_back(n);

  Face f0 = {
    lastVertexIndex, lastVertexIndex + 1, lastVertexIndex + 3,
    lastNormalIndex, lastNormalIndex, lastNormalIndex,
    0, 0, 0
  };

  Face f1 = {
    lastVertexIndex + 1, lastVertexIndex + 2, lastVertexIndex + 3,
    lastNormalIndex, lastNormalIndex, lastNormalIndex,
    0, 0, 0
  };

  mesh->faces.push_back(f0);
  mesh->faces.push_back(f1);
};
*/


bool VoxelsSplitter::processLod(std::shared_ptr<SplitTask> task, GridRef grid) {
  std::cout << "Split started" << std::endl;

  grid->init();
  GroupObject voxelized = this->decimate(task->target, grid);
  splitter::textureLOD(voxelized, 8);

  voxelized->name = "Lod";

  std::cout << "Calling callback" << std::endl;

  task->callback(voxelized, task->targetId, task->parentID, task->decimationLevel);
  voxelized->free(false);

  std::cout << "Split finished" << std::endl;

  return false;
};


bool VoxelsSplitter::split(GroupObject target, IdGenerator::ID parentId, unsigned int decimationLevel = 0, bool divideVertical = true) {
  std::cout << "Split id: " << parentId << std::endl;

  unsigned int polygonCount = 0;

  target->traverse([&](MeshObject mesh){
    polygonCount += mesh->faces.size();
  });

  IdGenerator::ID nextParent = parentId;

  if (polygonCount <= this->polygonsLimit) {
    if (this->onSave) {
      this->IDGen.next();
      nextParent = this->IDGen.id;

      GroupObject resultGroup = splitter::splitUV(target, 0);
      resultGroup->name = "Chunk";
      
      //target->name = "Chunk";
      this->onSave(resultGroup, nextParent, parentId, decimationLevel);

      std::cout << "Clearing the chunk" << std::endl;
      resultGroup->free();
    }

    return true;
  }

  
  if (this->onSave) {
    this->IDGen.next();
    nextParent = this->IDGen.id;

    
    std::shared_ptr<SplitTask> task = std::make_shared<SplitTask>();

    task->target = target;
    task->targetId = nextParent;
    task->parentID = parentId;
    task->decimationLevel = decimationLevel;
    task->callback = this->onSave;

    std::cout << "Creating a grid" << std::endl;

    GridRef grid = std::make_shared<VoxelGrid>();
    grid->isoLevel = this->gridSettings.isoLevel;
    grid->gridResolution = this->gridSettings.gridResolution;

    std::cout << "Creating a pool task" << std::endl;
    
    this->pool.create(
      bind(&VoxelsSplitter::processLod, this, std::placeholders::_1, std::placeholders::_2),
      task,
      grid
    );
    
    std::cout << "Waiting for result" << std::endl;

    this->pool.waitForSlot();
    

    std::cout << "Processing next task" << std::endl;

    /*
    GroupObject voxelized = this->decimate(target);
    // GroupObject voxelized = splitter::splitUV(this->decimate(target), 8);
    // GroupObject simplified = simplifier::modify(voxelized, 0.5f);
    // simplified->name = "Lod";
    splitter::textureLOD(voxelized, 8);
    voxelized->name = "Lod";

    this->onSave(voxelized, nextParent, parentId, decimationLevel);

    voxelized->free(false);
    */
  }

  std::cout << "Median split" << std::endl;
  

  GroupObject halfs = this->halfMesh(target, divideVertical);
  //target->free();

  for (GroupObject &half : halfs->children) {
    this->split(half, nextParent, decimationLevel + 1, !divideVertical);
  }

  return true;
};
