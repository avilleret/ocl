__kernel void process_texture_kernel(__read_only image2d_t im, __global bool *dst, int w, int h)
{
  sampler_t srcSampler = CLK_NORMALIZED_COORDS_FALSE | 
        CLK_ADDRESS_CLAMP_TO_EDGE |
        CLK_FILTER_NEAREST ;
        
	int i = get_global_id(0);
  int j = get_global_id(1);
  int2 coord = { i, j };
  float4 color = read_imagef( im, srcSampler, coord);
  int idx = i+w*j;
	dst[idx]= color.x > 0.5;
}
