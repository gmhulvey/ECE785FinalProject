#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/param.h>

#include "image_proc.h"
#include "is_options.h"
#include "yuv.h"

// #include "PCA9685_servo_driver.h"
#include "ansi_escapes.h"

unsigned char img2_bitplanes[ISS_STRIDE*ISS_HEIGHT*3/2];

unsigned char mask[ISS_STRIDE/2][ISS_HEIGHT/2];

#define CONTROL_LOOP_DIVIDER 2

void draw_overlay_info(YUV_IMAGE_T * i) {
  Draw_Circle(i, i->half_w, i->half_h, 10, &black, 0);
  Draw_Circle(i, i->half_w, i->half_h, 12, &orange, 0);
  Draw_Circle(i, i->half_w, i->half_h, 13, &orange, 0);
  Draw_Circle(i, i->half_w, i->half_h, 22, &black, 0);
  Draw_Circle(i, i->half_w, i->half_h, 24, &white, 0);
  Draw_Circle(i, i->half_w, i->half_h, 25, &white, 0);
}

void clear_term_screen(void) {
  printf("\033[2J");
}

void enable_runfast(void) {
  static const unsigned int x = 0x04086060;
  static const unsigned int y = 0x03000000;
  int r;
  asm volatile (
		"fmrx %0, fpscr \n\t" //r0 = FPSCR
		"and %0, %0, %1 \n\t" //r0 = r0 & 0x04086060
		"orr %0, %0, %2 \n\t" //r0 = r0 | 0x03000000
		"fmxr fpscr, %0 \n\t" //FPSCR = r0
		: "=r"(r)
		: "r"(x), "r"(y)
		);
}

void Draw_Match_Marker(YUV_IMAGE_T * i, int x, int y, int sep, YUV_T * color) {
  if (sep > 10)
    Draw_Rectangle(i, x, y, sep-2, sep-2, color, 0);
  else {
    Draw_Line(i, x-sep/2, y, x+sep/2, y, color);
    Draw_Line(i, x, y-sep/2, x, y+sep/2, color);
  }
}

int find_chroma_matches(YUV_IMAGE_T * i, YUV_T * tc, int * rc_col, int * rc_row, int sep){
  int col, row;
  int matches=0, diff;
  YUV_T color, prev_color={0,0,0};
  int c_col=0, c_row=0;
  YUV_T * match_color = &pink;
  YUV_T v_color;
  int half_col;
  int half_row;
  int index;
  int du, dv;

  for (row = 1; row <= i->h - 1; row += 2) {
    for (col = 1; col <= i->w - 1; col += 2) {
      half_row = row >> 1;
      half_col = col >> 1;
      index = half_col+half_row*i->half_s;
      // Get_Pixel_yuv(i, col, row, &color);

      color.u = i->bU[index];
      color.v = i->bV[index];
      // Identify pixels with right color
      // diff = Sq_UV_Difference_yuv(&color, tc);

      du = (int) color.u - (int) tc->u;
      dv = (int) color.v - (int) tc->v;

      diff = du*du + dv*dv;


      if (diff < color_threshold) {
        match_color = &pink;
        c_col += col;
        c_row += row;
        matches++;
        // Draw_Match_Marker(i, col, row, 2, match_color);
        // Just access single pixel of UV
        i->bU[index] = pink.u;
        i->bV[index] = pink.v;
      }
    } // for col
  } // for row
  
  if (matches > 0) {
    c_col /= matches;
    c_row /= matches;
    *rc_col = c_col;
    *rc_row = c_row;
  }
  
  return matches;
}

void process_image(uint8_t * src_bitplanes, uint8_t * dst_bitplanes) {
  static YUV_IMAGE_T img, img2;
  int translate_image = 0;
  int w=ISS_WIDTH, s=ISS_STRIDE, h=ISS_HEIGHT;
  // Default target color 
  static YUV_T target = {190, 116, 65};   // Green sock, day
  int Y_array_size = s*h;
  int UV_array_size = Y_array_size/4;
  int x, y; // coordinates
  int i;
  uint8_t * Y = src_bitplanes;
  YUV_T center_color;

  // initialize YUV_IMAGE_T data structures describing images
  YUV_Image_Init(&img, src_bitplanes, w, s, h); // original image

  Get_Pixel_yuv(&img, img.half_w, img.half_h, &center_color);
  if (update_target_color) {
    target = center_color;
    update_target_color = 0;
  }
  if (show_data > 1) {
    printf("Center pixel: (%3d, %3d, %3d)\n", center_color.y, center_color.u, center_color.v);
    printf("Target color: (%3d, %3d, %3d) Threshold %4d (ext=%d)\n",
	   target.y, target.u, target.v, color_threshold, extend_threshold);
  }

  // draw center circles
  draw_overlay_info(&img);
    
  // Find area matching target color
  int centroid_x, centroid_y, num_matches, offsetX, offsetY;
  num_matches = find_chroma_matches(&img, &target, &centroid_x, &centroid_y, chroma_subsample_sep);
  if (num_matches > 0) {  
    // Show centroid
    Draw_Circle(&img, centroid_x, centroid_y, 10, &white, 1);
    Draw_Line(&img, 0, 0, centroid_x, centroid_y, &white);
    Draw_Line(&img, img.w-1, 0, centroid_x, centroid_y, &white);

    offsetX = img.half_w - centroid_x;
    offsetY = img.half_h - centroid_y;
    if (show_data > 0) {
      printf("Match centroid at (%4d, %3d) with %5d samples\n",
	     centroid_x, centroid_y, num_matches);
      if (show_data > 1) 
	printf("Offset = %5d, %4d\n", offsetX, offsetY);
    }

#if 0
    // Correct image position
    if (imstab_digital) {
      // Copy bitplanes of image so far into another buffer
      YUV_Image_Copy(&img2, &img);
      YUV_Image_Fill(&img, &target);
      YUV_Translate_Image(&img, &img2, offsetX, offsetY, 0);
    }
    if (imstab_servo) {
      if ((loop % CONTROL_LOOP_DIVIDER) == 0) {
       	Update_Servos(offsetX, centroid_x, offsetY, centroid_y);
      }
      printf("Servo: pan = %5.2f, tilt = %5.2f\n", ServoPanDegree, ServoTiltDegree);
    }
#endif
  } else {
    printf("No match found\n");
  }    
}
