#include "base.hpp"
#include "mesh.hpp"
#include <vector>
#include <cmath>

namespace brdf
{

struct Vertex_PNC
{
   float x, y, z;
   float nx, ny, nz;
};

Mesh Mesh::make_sphere()
{
   Mesh m;

   m.vertex_decl
   .begin()
   .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
   .add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)//, true)
   .end();

   int sub_lat = 36;
   int sub_long = 36;

   bgfx::Memory const* vx_mem = bgfx::alloc((sub_lat * sub_long + 2) * sizeof(Vertex_PNC));
   Vertex_PNC* vertices = (Vertex_PNC*)vx_mem->data;
   vertices[0] = { 0.0,  1.0, 0.0, 0.0,  1.0, 0.0 }; // north pole
   vertices[1] = { 0.0, -1.0, 0.0, 0.0, -1.0, 0.0 }; // south pole
   uint v_idx = 2;
   for (int j = 0; j < sub_lat; ++j)
   {
      const float j_step = k_pi / static_cast<float>(sub_lat + 1);
      float phi = k_pi * 0.5f - (j_step * (j + 1));
      float c_phi = cos(phi);
      float s_phi = sin(phi);

      for (int i = 0; i < sub_long; ++i)
      {
         const float i_step = (2.f * k_pi) / static_cast<float>(sub_long);
         float theta = i_step * i;
         float c_theta = cos(theta);
         float s_theta = sin(theta);

         vertices[v_idx] = { c_phi * c_theta, s_phi, c_phi * s_theta, c_phi * c_theta, s_phi, c_phi * s_theta };
         v_idx++;
      }
   }
   m.m_vbh = bgfx::createVertexBuffer(vx_mem, m.vertex_decl);

   int sub_lat_minus_one = sub_lat - 1;

   size_t tri_count = sub_long * (2 + 2 * sub_lat_minus_one);
   bgfx::Memory const * idx_mem = bgfx::alloc(tri_count * 3 * sizeof(uint16_t));
   uint16_t* indices = (uint16_t*)idx_mem->data;
   uint32_t idx = 0;
   for (int i = 0; i < sub_long; ++i) // north cape
   {
      indices[idx++] = (0);
      indices[idx++] = (2 + i);
      if (i == (sub_long - 1))
         indices[idx++] = (2 + 0);
      else
         indices[idx++] = (2 + i + 1);
   }
   for (int j = 0; j < sub_lat_minus_one; ++j)
   {
      const uint idx_offset = 2 + sub_long * j;
      const uint upper_idx_offset = 2 + sub_long * (j + 1);
      for (int i = 0; i < sub_long; ++i) // south cape
      {
         uint next = (i + 1) % sub_long;
         indices[idx++] = (idx_offset + i);
         indices[idx++] = (upper_idx_offset + i);
         indices[idx++] = (idx_offset + next);

         indices[idx++] = (upper_idx_offset + i);
         indices[idx++] = (upper_idx_offset + next);
         indices[idx++] = (idx_offset + next);
      }
   }
   const uint south_idx_offset = 2 + sub_lat_minus_one * sub_long;
   for (int i = 0; i < sub_long; ++i) // south cape
   {
      indices[idx++] = (1);
      if (i == (sub_long - 1))
         indices[idx++] = (south_idx_offset);
      else
         indices[idx++] = (south_idx_offset + i + 1);
      indices[idx++] = (south_idx_offset + i);
   }
   m.m_ibh = bgfx::createIndexBuffer(idx_mem);
   return m;
}

void Mesh::bind()
{
   bgfx::setVertexBuffer(0, m_vbh);
   bgfx::setIndexBuffer(m_ibh);
}

Mesh::~Mesh()
{
   //bgfx::destroyIndexBuffer(m_ibh);
   //bgfx::destroyVertexBuffer(m_vbh);
}

}