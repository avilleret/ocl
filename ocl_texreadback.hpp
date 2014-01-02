/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    ocl_texreadback - simple test

    Copyright (c) 1997-2000 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_OCL_TEXREADBACK_H_
#define _INCLUDE__GEM_OCL_TEXREADBACK_H_

#include "Base/GemShape.h"
#include "Gem/State.h"
#include "Gem/Exception.h"

#include <iostream>
#include <fstream>
#include <sstream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif


/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    ocl_texreadback

    simple opencl test

KEYWORDS
    geo

DESCRIPTION

-----------------------------------------------------------------*/
class GEM_EXTERN ocl_texreadback : public GemShape
{
    CPPEXTERN_HEADER(ocl_texreadback, GemShape);

    public:

        //////////
        // Constructor
    	ocl_texreadback(t_floatarg size);

    protected:

    	//////////
    	// Destructor
    	virtual ~ocl_texreadback();

    	//////////
    	// Do the rendering
    	virtual void 	renderShape(GemState *state);
      cl_context CreateContext();
      cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device);
      cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName);
      bool CreateMemObjects(cl_context context, GLuint texture, GLuint vbo, cl_mem *p_cl_vbo_mem, cl_mem *p_cl_tex_mem);
      void Cleanup();
             
             
      //////////
      // Create OpenCL context *AFTER* OpenGL Context
      void         startRendering(void);
      //////////
      // Clean up OpenCL context
      void         stopRendering(void);
    
    private:
    
      void performQueries();
      void initTexture( int width, int height );
      GLuint initVBO(int vbolen );
      cl_int computeTexture(), computeVBO();
      void renderVBO( int vbolen );
      void displayTexture(int w, int h);
      
      GLuint tex, vbo;
      int vbolen, imWidth, imHeight;

      cl_context context;
      cl_command_queue commandQueue;
      cl_program program;
      cl_device_id device;
      cl_kernel kernel, tex_kernel;
      cl_mem cl_vbo_mem, cl_tex_mem;
      cl_mem memObjects[3];
      
      float result[ARRAY_SIZE];
      float a[ARRAY_SIZE];
      float b[ARRAY_SIZE];
      
      bool m_opencl_is_init;
      
};

#endif	// for header file
