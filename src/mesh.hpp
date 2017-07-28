#pragma once

#include <bgfx/bgfx.h>

namespace brdf
{

struct Mesh
{
   ~Mesh();
   static Mesh make_sphere();

   void bind();
private:
   bgfx::VertexDecl vertex_decl;

   bgfx::VertexBufferHandle m_vbh;
   bgfx::IndexBufferHandle m_ibh;
};

}