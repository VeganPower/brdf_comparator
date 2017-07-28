/*
 * Copyright 2011-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "base.hpp"
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"

#include "mesh.hpp"

#include "file_notifier.hpp"

struct ApiState
{
   int dragging_over_ui = 0;
   int right_scroll_area = 0;
   bool view_fullscreen = false;
   bool do_diffuse = false;
   bool do_specular = false;
   bool show_light_color_wheel = false;
   bool show_albedo_color_wheel = false;
   float roughtness = 1.0;
   float fresnel0 = 1.0;
   float light_dir[2] = { 0.0, k_pi * 0.5f };
   float light_col[3] = { 1.0, 1.0, 1.0 };
   float albedo[3] = { 0.75, 0.50, 0.25 };
};

void render_menu(uint32_t width, uint32_t height, entry::MouseState const& mouse_state, ApiState& options)
{
   uint8_t imgui_buttons = (mouse_state.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
                           | (mouse_state.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
                           | (mouse_state.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0);
   const bool mouse_over_gui = imguiMouseOverArea();
   if (mouse_over_gui)
   {
      options.dragging_over_ui = imgui_buttons != 0;
   }
   if (imgui_buttons == 0)
   {
      options.dragging_over_ui = false;
   }
   imguiBeginFrame(mouse_state.m_mx
                   , mouse_state.m_my
                   , imgui_buttons
                   , mouse_state.m_mz
                   , uint16_t(width)
                   , uint16_t(height)
                  );


   imguiBeginScrollArea("", width - 256 - 10, 10, 256, 700, &options.right_scroll_area);
   imguiSlider("Roughtness", options.roughtness, 0.0f, 1.0f, 0.01f, options.view_fullscreen);
   imguiSlider("F0", options.fresnel0, 0.0f, 1.0f, 0.01f, options.view_fullscreen);
   imguiLabel("Directional light:");
   imguiBool("Diffuse",  options.do_diffuse);
   imguiBool("Specular", options.do_specular);
   const bool do_direct_lighting = true;//options.do_diffuse || options.do_specular;

   imguiSlider("Light elevation", options.light_dir[0], -90.f, 90.f, 1.f, do_direct_lighting);
   imguiSlider("Light direction", options.light_dir[1], -180.f, 180.f, 1.f, do_direct_lighting);
   imguiColorWheel("Light color:", options.light_col, options.show_light_color_wheel, 0.6f, do_direct_lighting);
   imguiColorWheel("Albedo:", options.albedo, options.show_albedo_color_wheel, 0.6f, true);
   imguiEndScrollArea();

   imguiEndFrame();
}

int _main_(int _argc, char** _argv)
{
   static const uint32_t k_param1 = 5;
   static const uint32_t k_param2 = 10;
   static const uint32_t k_sphere_count = k_param1 * k_param2;
   Args args(_argc, _argv);

   uint32_t m_width;
   uint32_t m_height;
   uint32_t m_debug;
   uint32_t m_reset;

   bgfx::ProgramHandle m_program;
   int64_t m_timeOffset;

   bgfx::UniformHandle uniform[k_sphere_count];
   brdf::Mesh m_sphere;
   uint16_t view_X = 0;
   uint16_t view_Y = 0;


   ApiState options;
   {
      m_width  = 1280;
      m_height = 720;
      m_debug  = BGFX_DEBUG_TEXT;
      m_reset  = BGFX_RESET_VSYNC | BGFX_RESET_SRGB_BACKBUFFER;

      bgfx::init(args.m_type, args.m_pciId);
      bgfx::reset(m_width, m_height, m_reset);

      // Enable debug text.
      bgfx::setDebug(m_debug);

      imguiCreate();

      // Set view 0 clear state.
      for (int i = 0; i < k_sphere_count; ++i)
      {
         bgfx::setViewClear(i
                            , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
                            , 0x303030ff
                            , 1.0f
                            , 0);
      }

      m_program = loadProgram("vs_cubes", "fs_cubes");

      m_sphere = brdf::Mesh::make_sphere();

      m_timeOffset = bx::getHPCounter();

      for (int i = 0; i < k_sphere_count; ++i)
      {
         uniform[i] = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 4);
      }
   }

   FileNotifier file_notifier("shaders\\dx11");
   auto file_callback = [&m_program]
   {
      //bgfx::destroyProgram(m_program);
      m_program = loadProgram("vs_cubes", "fs_cubes");
   };

   file_notifier.add_notification("vs_cubes.bin", file_callback, 1 );
   file_notifier.add_notification("fs_cubes.bin", file_callback, 1 );

   {
      entry::MouseState mouse_state;
      while (!entry::processEvents(m_width, m_height, m_debug, m_reset, &mouse_state))
      {

         render_menu(m_width, m_height, mouse_state, options);
         int64_t now = bx::getHPCounter();
         static int64_t last = now;
         const int64_t frameTime = now - last;
         last = now;
         const double freq = double(bx::getHPFrequency() );
         const double toMs = 1000.0 / freq;

         float time = (float)( (now - m_timeOffset) / double(bx::getHPFrequency() ) );

         // Use debug font to print information about this example.
         bgfx::dbgTextClear();
         bgfx::dbgTextPrintf(0, 0, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

         uint16_t width = uint16_t((m_width - 256) / k_param2);
         uint16_t height = uint16_t(m_height / k_param1);

         float at[3]  = { 0.0f, 0.0f,  0.0f };
         float eye[3] = { 0.0f, 0.0f, -3.0f };

         if (!options.dragging_over_ui)
         {
            if (mouse_state.m_buttons[entry::MouseButton::Left] && !options.view_fullscreen)
            {
               view_X = mouse_state.m_mx / width;
               view_Y = mouse_state.m_my / height;
               options.view_fullscreen = true;
               options.roughtness = float(view_X) / (k_param2 - 1.f);
               options.fresnel0 = float(view_Y) / (k_param1 - 1.f);
            }
            if (mouse_state.m_buttons[entry::MouseButton::Right])
            {
               options.view_fullscreen = false;
            }
         }

         float view[16];
         bx::mtxLookAt(view, eye, at);

         float proj[16];
         {
            float light_dir[3];
            float phi = options.light_dir[0] * k_pi / 180.f;
            float theta = options.light_dir[1] * k_pi / 180.f;
            float c_phi = cos(phi);
            light_dir[0] = sin(theta) * c_phi;
            light_dir[1] = sin(phi);
            light_dir[2] = -cos(theta) * c_phi;
            float spacing = 3.0f;
            float params[16] =
            {
               eye[0], eye[1], eye[2], options.fresnel0,
               light_dir[0], light_dir[1], light_dir[2], options.roughtness,
               options.light_col[0], options.light_col[1], options.light_col[2], (float)options.do_specular,
               options.albedo[0], options.albedo[1], options.albedo[2], (float)options.do_diffuse
            };
            if (options.view_fullscreen)
            {
               bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
               bgfx::setViewRect(0, 0, 0, m_width, m_height);
               bgfx::setViewTransform(0, view, proj);
               bgfx::setUniform(uniform[0], params, 4);
               m_sphere.bind();
               bgfx::setState(BGFX_STATE_DEFAULT);

               bgfx::submit(0, m_program);
            }
            else
            {
               uint8_t view_idx = 0;
               bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
               for (uint32_t xx = 0; xx < k_param2; ++xx)
               {
                  float roughtness = float(xx) / (k_param2 - 1.f);
                  for (uint32_t yy = 0; yy < k_param1; ++yy)
                  {
                     float fresnel0 = float(yy) / (k_param1 - 1.f);

                     bgfx::setViewRect(view_idx, width * xx, height * yy, width, height);
                     bgfx::setViewTransform(view_idx, view, proj);

                     params[3] = fresnel0;
                     params[7] = roughtness;
                     bgfx::setUniform(uniform[view_idx], params, 4);

                     m_sphere.bind();
                     bgfx::setState(BGFX_STATE_DEFAULT);
                     bgfx::submit(view_idx++, m_program);
                  }
               }
            }
         }
		 file_notifier.pool();
		 bgfx::frame();
      }
   }

   {
      for (int i = 0; i < k_sphere_count; ++i)
      {
         bgfx::destroyUniform(uniform[i]);
      }
      // Cleanup.
      bgfx::destroyProgram(m_program);

      // Shutdown bgfx.
      bgfx::shutdown();
      return 0;
   }

};
