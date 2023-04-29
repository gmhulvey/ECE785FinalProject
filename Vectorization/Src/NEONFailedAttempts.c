

int find_chroma_matches(YUV_IMAGE_T * i, YUV_T * tc, int * rc_col, int * rc_row, int sep){
  int col, row;
  int matches=0, diff_comp;
  YUV_T color; //, prev_color={0,0,0};
  int c_col=0, c_row=0;
  YUV_T * match_color = &pink;
  // YUV_T v_color;
  int color_u[4],color_v[4],tc_u[4],tc_v[4],count = 0;

  // Target Color tc = {190, 116, 65}
  int32x2_t vector_diff = vdup_n_s32(0);
  int16x4_t vector_uimage = vdup_n_s16(0);
  int32x2_t vector_thresh = vld1_s32(&color_threshold);
  
  // for (row = sep/2; row <= i->h - sep/2; row += sep) {
  //   for (col = sep/2; col <= i->w - sep/2; col += sep) {

  for (row = 1; row <= i->h - 1; row += 2) {
    for (col = 1; col <= i->w - 1; col += 2) {
      // UNVECTORIZED CODE
      Get_Pixel_yuv(i, col, row, &color);
      // Identify pixels with right color
      // diff = Sq_UV_Difference_yuv(&color, tc);

      // Vectorized NEON Functions
      //  vector_du, vector_dv, vector_uimage, vector_vimage, vector_utarget, vector_vtarget;
      // int32x2_t vector_squ, vector_sqv, vector_diff, vector_thresh;
      // uint32x2_t vector_comp;
        color_u[count] = color.u;
        color_v[count] = color.v;
        tc_u[count] = tc->u;
        tc_v[count] = tc->v;
        count++;
      if(count == 4){
        vector_uimage = vld1_s16(&color_u);
        int16x4_t vector_vimage = vld1_s16(&color_v);
        int16x4_t vector_utarget = vld1_s16(&tc_u);
        int16x4_t vector_vtarget = vld1_s16(&tc_v);

        int16x4_t vector_du = vsub_s16(vector_uimage, vector_utarget);
        int16x4_t vector_dv = vsub_s16(vector_vimage, vector_vtarget);

        int32x2_t vector_squ = vmul_s32((int32x2_t)vector_du, (int32x2_t)vector_du);
        int32x2_t vector_sqv = vmul_s32((int32x2_t)vector_dv, (int32x2_t)vector_dv);

        vector_diff = vadd_s32(vector_squ, vector_sqv);

        uint32x2_t vector_comp = vcle_s32(vector_diff, vector_thresh);

        diff_comp = vget_lane_u32(vector_comp,0);

        if (diff_comp) {
          match_color = &pink;
          c_col += col;
          c_row += row;
          matches++;
          Draw_Match_Marker(i, col, row, 2, match_color);
        }
        count =0;
      }

      // NEON code Start



    } // for col
  } // for row



// 4/27/23

// int find_chroma_matches(YUV_IMAGE_T * i, YUV_T * tc, int * rc_col, int * rc_row, int sep){
//   int col, row;
//   int matches=0, diff;
//   YUV_T color; //, prev_color={0,0,0};
//   int c_col=0, c_row=0;
//   YUV_T * match_color = &pink;

// #pragma clang loop vectorize(assume_safety)
//   for (row = 1; row <= i->h - 1; row += 2) {
//     #pragma clang loop unroll_count(16)
//     for (col = 1; col <= i->w - 1; col += 2) {
//       Get_Pixel_yuv(i, col, row, &color);
//       // Identify pixels with right color

//       diff = Sq_UV_Difference_yuv(&color, tc);

//       if (diff < color_threshold) {
// 	match_color = &pink;
// 	c_col += col;
// 	c_row += row;
// 	matches++;
// 	Draw_Match_Marker(i, col, row, sep, match_color);
//       }
//     } // for col
//   } // for row
  
//   if (matches > 0) {
//     c_col /= matches;
//     c_row /= matches;
//     *rc_col = c_col;
//     *rc_row = c_row;
//   }
  
//   return matches;
// }

// int find_chroma_matches(YUV_IMAGE_T * i, YUV_T * tc, int * rc_col, int * rc_row, int sep){
//   int col, row;
//   int matches=0;
//   uint32_t diff_comp;
//   YUV_T color; //, prev_color={0,0,0};
//   int c_col=0, c_row=0;
//   YUV_T * match_color = &pink;
//   // YUV_T v_color;
//   int color_u[4],color_v[4],tc_u[4],tc_v[4],count = 0;
//   int16x4_t ones_array = vld1_dup_s16(1);

//   // Target Color tc = {190, 116, 65}
//   int32x2_t vector_diff = vdup_n_s32(0);
//   int16x4_t vector_uimage,vector_vimage,vector_utarget,vector_vtarget,vector_du,vector_dv;
//   int32x2_t vector_thresh = vld1_s32(&color_threshold);
//   int32x2_t vector_squ,vector_sqv;
//   uint32x2_t vector_comp;
  
//   // for (row = sep/2; row <= i->h - sep/2; row += sep) {
//   //   for (col = sep/2; col <= i->w - sep/2; col += sep) {

//   for (row = 1; row <= i->h - 1; row += 2) {
//     for (col = 1; col <= i->w - 1; col += 2) {
//       // UNVECTORIZED CODE
//       Get_Pixel_yuv(i, col, row, &color);
//       // Identify pixels with right color
//       // diff = Sq_UV_Difference_yuv(&color, tc);

//       // Vectorized NEON Functions
//         color_u[count] = color.u;
//         color_v[count] = color.v;
//         tc_u[count] = tc->u;
//         tc_v[count] = tc->v;
//         count++;
//       if(count == 4){
//         vector_uimage = vld1_s16(&color_u);
//         vector_vimage = vld1_s16(&color_v);
//         vector_utarget = vld1_s16(&tc_u);
//         vector_vtarget = vld1_s16(&tc_v);

//         vector_du = vsub_s1sep6(vector_uimage, vector_utarget);
//         vector_dv = vsub_s16(vector_vimage, vector_vtarget);

//         vector_squ = vmul_s32((int32x2_t)vector_du, (int32x2_t)vector_du);
//         vector_sqv = vmul_s32((int32x2_t)vector_dv, (int32x2_t)vector_dv);

//         vector_diff = vadd_s32(vector_squ, vector_sqv);

//         vector_comp = vclt_s32(vector_diff, vector_thresh);

//         diff_comp = vget_lane_u32(vector_comp,0);
//         // printf("diff_comp = %d\n", diff_comp);
//         if (diff_comp) {
//           match_color = &pink;
//           c_col += col;
//           c_row += row;
//           matches++;
//           Draw_Match_Marker(i, col, row, 2, match_color);
//         }
//         count =0;
//       }

//       // NEON code Start
//     } // for col
//   } // for row
//   if (matches > 0) {
//     c_col /= matches;
//     c_row /= matches;
//     *rc_col = c_col;
//     *rc_row = c_row;
//   }
  
//   return matches;
// }