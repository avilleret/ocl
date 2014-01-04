////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "ocl_texreadback.hpp"

CPPEXTERN_NEW_WITH_ONE_ARG(ocl_texreadback, t_floatarg, A_DEFFLOAT);

/// 
// Demonstrate usage of the GL Object querying capabilities
//
void ocl_texreadback :: performQueries() {
	cl_int errNum;
	std::cout << "Performing queries on OpenGL objects:" << std::endl;
	// example usage of getting information about a GL memory object
	cl_gl_object_type obj_type;
	GLuint objname;
	errNum = clGetGLObjectInfo( cl_tex_mem,  &obj_type, &objname );
	if( errNum != CL_SUCCESS ) {
		std::cerr << "Failed to get object information" << std::endl;
	} else {
		if( obj_type == CL_GL_OBJECT_TEXTURE2D ) {
			std::cout << "Queried a texture object succesfully." << std::endl;
			std::cout << "Object name is: " << objname << std::endl; 
		}
		
	}

	// Example usage of how to get information about the texture object
	GLenum param;
	size_t param_ret_size;
	errNum = clGetGLTextureInfo( cl_tex_mem, CL_GL_TEXTURE_TARGET, sizeof( GLenum ), &param, &param_ret_size );
	if( errNum != CL_SUCCESS ) {
		std::cerr << "Failed to get texture information" << std::endl;
	} else {
		// we have set it to use GL_TEXTURE_RECTANGLE_ARB.  We expect it to be reflectedin the query here
		if( param == GL_TEXTURE_RECTANGLE_ARB ) {
			std::cout << "Texture rectangle ARB is being used." << std::endl;
		}
	}
}

///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
cl_context ocl_texreadback :: CreateContext()
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    // First, select an OpenCL platform to run on.  For this example, we
    // simply choose the first available platform.  Normally, you would
    // query for all available platforms and select the most appropriate one.
    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }

    // Next, create an OpenCL context on the platform.  Attempt to
    // create a GPU-based context, and if that fails, try to create
    // a CPU-based context.
    cl_context_properties contextProperties[] =
    {
#ifdef _WIN32
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
		CL_GL_CONTEXT_KHR,
		(cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR,
		(cl_context_properties)wglGetCurrentDC(),
#elif defined( __GNUC__)
		CL_CONTEXT_PLATFORM, (cl_context_properties)firstPlatformId, 
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
#elif defined(__APPLE__) 
		//todo
#endif
        0



    };
	cl_uint uiDevCount;
    cl_device_id* cdDevices;
	// Get the number of GPU devices available to the platform
    errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, 0, NULL, &uiDevCount);

    // Create the device list
    cdDevices = new cl_device_id [uiDevCount];
    errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, uiDevCount, cdDevices, NULL);


    context = clCreateContext(contextProperties, 1, &cdDevices[0], NULL, NULL, &errNum);
	//// alternate:
    //context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
    //                                  NULL, NULL, &errNum);

    if (errNum != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
                                          NULL, NULL, &errNum);
        if (errNum != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return NULL;
        }
    }

    return context;
}

///
//  Create a command queue on the first device available on the
//  context
//
cl_command_queue ocl_texreadback :: CreateCommandQueue(cl_context context, cl_device_id *device)
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    // First get the size of the devices buffer
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
        return NULL;
    }

    if (deviceBufferSize <= 0)
    {
        std::cerr << "No devices available.";
        return NULL;
    }

    // Allocate memory for the devices buffer
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
    if (errNum != CL_SUCCESS)
    {
        delete [] devices;
        std::cerr << "Failed to get device IDs";
        return NULL;
    }

    // In this example, we just choose the first available device.  In a
    // real program, you would likely use all available devices or choose
    // the highest performance device based on OpenCL device queries
    commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    if (commandQueue == NULL)
    {
        delete [] devices;
        std::cerr << "Failed to create commandQueue for device 0";
        return NULL;
    }

    *device = devices[0];
    delete [] devices;
    return commandQueue;
}

///
//  Create an OpenCL program from the kernel source file
//
cl_program ocl_texreadback :: CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
    cl_int errNum;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1,
                                        (const char**)&srcStr,
                                        NULL, NULL);
    if (program == NULL)
    {
        std::cerr << "Failed to create CL program from source." << std::endl;
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

///
//  Create memory objects used as the arguments to kernels in OpenCL
//  The memory objects are created from existing OpenGL buffers and textures
//
bool ocl_texreadback :: CreateMemObjects(cl_context context, GLuint texture, cl_mem *p_cl_tex_mem, cl_mem *p_cl_binBuf_mem)
{
	cl_int errNum;
	
	*p_cl_tex_mem = clCreateFromGLTexture2D(context, CL_MEM_READ_ONLY, GL_TEXTURE_RECTANGLE_ARB, 0, texture, &errNum );
	if( errNum != CL_SUCCESS )
	{
		std::cerr<< "Failed creating memory from GL texture." << std::endl;
		return false;
	}
  
  *p_cl_binBuf_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(bool) * m_width * m_height, NULL, NULL);

  if ( cl_bin_mem == NULL)
  {
    std::cerr << "Error creating memory objects." << std::endl;
    return false;
  }
  return true;
}

///
//  Cleanup any created OpenCL resources
//
void ocl_texreadback :: Cleanup()
{
    if (commandQueue != 0){
        clReleaseCommandQueue(commandQueue);
        commandQueue=0;
    }

    if (program != 0){
        clReleaseProgram(program);
        program=0;
    }

    if (context != 0){
        clReleaseContext(context);
        context=0;
    }

    if( tex_kernel != 0 ){
      clReleaseKernel(tex_kernel);
      tex_kernel=0;
    }

    if( cl_tex_mem != 0 ){
      clReleaseMemObject(cl_tex_mem);
      cl_tex_mem=0;
    }

    post("Cleanup() complete");
}

///
// Use OpenCL to process texture data
cl_int ocl_texreadback :: computeTexture()
{
	cl_int errNum;

    errNum = clSetKernelArg(tex_kernel, 0, sizeof(cl_mem), &cl_tex_mem);
    errNum = clSetKernelArg(tex_kernel, 1, sizeof(cl_mem), &cl_bin_mem);
    errNum = clSetKernelArg(tex_kernel, 2, sizeof(cl_int), &m_width);
    errNum = clSetKernelArg(tex_kernel, 3, sizeof(cl_int), &m_height);
	
	size_t tex_globalWorkSize[2] = { m_width, m_height };
	size_t tex_localWorkSize[2] = { 32, 4 } ;

	glFinish();
	errNum = clEnqueueAcquireGLObjects(commandQueue, 1, &cl_tex_mem, 0, NULL, NULL );

    errNum = clEnqueueNDRangeKernel(commandQueue, tex_kernel, 2, NULL,
                                    tex_globalWorkSize, tex_localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
    }
	errNum = clEnqueueReleaseGLObjects(commandQueue, 1, &cl_tex_mem, 0, NULL, NULL );
  
	clFinish(commandQueue);
	return 0;
}


/////////////////////////////////////////////////////////
//
// ocl_texreadback
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
ocl_texreadback :: ocl_texreadback(t_floatarg size)
        : GemShape(size),
        texture(1),
        m_width(-1),
        m_height(-1),
        context(0),
        commandQueue(0),
        program(0),
        device(0),
        tex_kernel(0),
        cl_tex_mem(0),
        cl_bin_mem(NULL),
        m_binBuf(NULL),
        m_binaryImage(NULL)
{
  m_opencl_is_init=false;
  
  m_outTexID = outlet_new(this->x_obj, &s_float);
}

void ocl_texreadback :: initOpenCL(GemState *state)
{
    if ( m_width < 0 || m_height < 0 ) return;

    // Create an OpenCL context on first available platform
    context = CreateContext();
    if (context == NULL)
    {
        error("Failed to create OpenCL context.");
        m_opencl_is_init = false;
        return;
    }

    // Create a command-queue on the first device available
    // on the created context
    commandQueue = CreateCommandQueue(context, &device);
    if (commandQueue == NULL)
    {
        Cleanup();
        error("Failed to create command cue.");
        m_opencl_is_init = false;
        return;
    }

    // Create OpenCL program from *.cl kernel source
    program = CreateProgram(context, device, "ocl_texreadback.cl");
    if (program == NULL)
    {
        Cleanup();
        error("Failed to create program");
        m_opencl_is_init = false;
        return;
    }

    tex_kernel = clCreateKernel(program, "process_texture_kernel", NULL);
    if (tex_kernel == NULL)
    {
        Cleanup();
        error("Failed to create texture kernel");
        m_opencl_is_init = false;
        return;
    }

    // Create memory objects that will be used as arguments to
    // kernel
    if (!CreateMemObjects(context, texture, &cl_tex_mem, &cl_bin_mem))
    {
        Cleanup();
        error("Failed to create mem objects");
        m_opencl_is_init = false;
        return;
    }
    m_opencl_is_init = true;
    
    performQueries();
}

void ocl_texreadback :: stopRendering(void){
  Cleanup();
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
ocl_texreadback :: ~ocl_texreadback()
{
  Cleanup();
}

/////////////////////////////////////////////////////////
// renderShape
//
/////////////////////////////////////////////////////////
void ocl_texreadback :: renderShape(GemState *state)
{
    cl_int errNum;
    
    pixBlock *pix;
    state->get(GemState::_PIX, pix);
    int size=m_width * m_height;
    
    if ( !pix ) return;
    
    if ( m_width != pix->image.xsize || m_height != pix->image.ysize ){
      // release previous data
      if (m_binaryImage)  {
        m_binaryImage->clear();
        delete m_binaryImage;
        m_binaryImage = NULL;
      }
      m_binaryImage = new imageStruct;
      m_width = pix->image.xsize;
      m_height = pix->image.ysize;
      m_binaryImage->xsize = m_width;
      m_binaryImage->ysize = m_height;
      m_binaryImage->setCsizeByFormat(GL_LUMINANCE);
      m_binaryImage->upsidedown = pix->image.upsidedown;

      m_binaryImage->allocate(m_binaryImage->xsize * m_binaryImage->ysize * m_binaryImage->csize);
      
      size = m_width * m_height;
      
      if ( m_binBuf ){
        delete m_binBuf;
        m_binBuf=NULL;
      }
      m_binBuf = new bool[m_width * m_height];
    }
    
    if ( !m_opencl_is_init ){
      initOpenCL(state);
      //~error("OpenCL is not initialized properly");
      return;
    }
    
    computeTexture();
    
    errNum = clEnqueueReadBuffer(commandQueue, cl_bin_mem, CL_TRUE,
                                 0, size * sizeof(bool), m_binBuf,
                                 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        error("Error reading result buffer.");
    } else {
      
      if ( m_binaryImage == NULL ){
        error("can't get image pointer\n");
        return;
      }
      
      unsigned char* ptr = m_binaryImage->data;
      for ( int i = 0; i < size; i++ ){
          ptr[i] = m_binBuf[i]*255;
      }
      m_pixBlock.image = *m_binaryImage;
      m_pixBlock.newimage = true;
    }
    state->set(GemState::_PIX, &m_pixBlock);
}

void ocl_texreadback :: obj_setupCallback(t_class *classPtr){
  CPPEXTERN_MSG (classPtr, "extTexture", extTextureMess);
}

void ocl_texreadback :: extTextureMess(t_symbol*s, int argc, t_atom*argv)
{
  int index=5;
  switch(argc){
  case 5:
    if(A_FLOAT!=argv[4].a_type)break;
    m_extUpsidedown=atom_getint(argv+4);
  case 4:
    index=4;
    if(A_FLOAT!=argv[3].a_type)break;
    m_extType=atom_getint(argv+3);
  case 3:
    index=3;
    if(A_FLOAT!=argv[2].a_type)break;
    index=2;
    if(A_FLOAT!=argv[1].a_type)break;
    m_width =atom_getfloat(argv+1);
    m_height=atom_getfloat(argv+2);
  case 1:
    index=1;
    if(A_FLOAT!=argv[0].a_type)break;
    m_extTextureObj=atom_getint(argv+0);
    index=0;
    return;
  default:
    error("arguments: <texId> [<width> <height> [<type> [<upsidedown>]]]");
    return;
  }
  if(index)
    error("invalid type of argument #%d", index);
}
