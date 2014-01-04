__kernel void init_texture_kernel(image2d_t im, int w, int h)
{
  
  sampler_t srcSampler = CLK_NORMALIZED_COORDS_FALSE | 
        CLK_ADDRESS_CLAMP_TO_EDGE |
        CLK_FILTER_NEAREST ;
        
	int2 coord = { get_global_id(0), get_global_id(1) };
  float4 color = read_imagef( im, srcSampler, coord);
  color = 1. - color;
	write_imagef( im, coord, color );
}
