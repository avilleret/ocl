/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    ocl_test - simple test

    Copyright (c) 1997-2000 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_GEOS_SQUARE_H_
#define _INCLUDE__GEM_GEOS_SQUARE_H_

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
    ocl_test

    simple opencl test

KEYWORDS
    geo

DESCRIPTION

-----------------------------------------------------------------*/
class GEM_EXTERN ocl_test : public GemShape
{
    CPPEXTERN_HEADER(ocl_test, GemShape);

    public:

        //////////
        // Constructor
    	ocl_test(t_floatarg size);

    protected:

    	//////////
    	// Destructor
    	virtual ~ocl_test();

    	//////////
    	// Do the rendering
    	virtual void 	renderShape(GemState *state);
      cl_context CreateContext();
      cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device);
      cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName);
      bool CreateMemObjects(cl_context context, cl_mem memObjects[3],
                      float *a, float *b);
      void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel, cl_mem memObjects[3]);
    
    private:
      cl_context context;
      cl_command_queue commandQueue;
      cl_program program;
      cl_device_id device;
      cl_kernel kernel;
      cl_mem memObjects[3];
      
      float result[ARRAY_SIZE];
      float a[ARRAY_SIZE];
      float b[ARRAY_SIZE];
      
};

#endif	// for header file
