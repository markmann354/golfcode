//
//        Program to test PatchCorrespondenceGen class
//        Graham Thomas, Jan 2007
//
//
//





int
flesh_hue_finder(int red, int green, int blue)
{

             //What is the hue of each pixel?
             int smallestrgb = 0;
             int largestrgb = 0;
             int h = 0;

                   smallestrgb = red;
                   if (green<smallestrgb)
                   { smallestrgb = green;
                   }
                   if (blue<smallestrgb)
                   { smallestrgb = blue;
                   }

                   largestrgb = red;
                   if (green>largestrgb)
                   { largestrgb = green;
                   }
                   if (blue>largestrgb)
                   { largestrgb = blue;
                   }

                   int delta = largestrgb-smallestrgb;

                   if (delta == 0)
                   { h = -60;       }
                   else if ((largestrgb-smallestrgb) != 0)
                   {
                       int Rr =0;
                       int Gg =0;
                       int Bb =0;
                       Rr =red*60/delta;
                       Gg =green*60/delta;
                       Bb =blue*60/delta;

                       if (red>green && red>blue)
                       {   h = (Gg-Bb);}
                       if (green>red && green>blue)
                       {   h = (120+(Bb-Rr));}
                       if (blue>green && blue>red)
                       {   h = (240+Rr-Gg);}
                       if (red==blue && red==largestrgb)
                       {   h = 300;}
                       if (red==green && green==largestrgb)
                       {   h = 60;}
                       if (green==blue && blue==largestrgb)
                       {   h = 180;}
                   }
                   ////at this point h becomes hue
                   if (h > 180)
                   {     h -= 360;}
               return (h);
       }


int hue_finder(int red, int green, int blue)
{

             //What is the hue of each pixel?
             int smallestrgb = 0;
             int largestrgb = 0;
             int h = 0;

                   smallestrgb = red;
                   if (green<smallestrgb)
                   { smallestrgb = green;
                   }
                   if (blue<smallestrgb)
                   { smallestrgb = blue;
                   }

                   largestrgb = red;
                   if (green>largestrgb)
                   { largestrgb = green;
                   }
                   if (blue>largestrgb)
                   { largestrgb = blue;
                   }

                   int delta = largestrgb-smallestrgb;

                   if (delta == 0)
                   { h = -60;       }
                   else if ((largestrgb-smallestrgb) != 0)
                   {
                       int Rr =0;
                       int Gg =0;
                       int Bb =0;
                       Rr =red*60/delta;
                       Gg =green*60/delta;
                       Bb =blue*60/delta;

                       if (red>green && red>blue)
                       {   h = (Gg-Bb);}
                       if (green>red && green>blue)
                       {   h = (120+(Bb-Rr));}
                       if (blue>green && blue>red)
                       {   h = (240+Rr-Gg);}
                       if (red==blue && red==largestrgb)
                       {   h = 300;}
                       if (red==green && green==largestrgb)
                       {   h = 60;}
                       if (green==blue && blue==largestrgb)
                       {   h = 180;}
                   }
                   ////at this point h becomes hue
                   if (h < 0)
                   {     h += 360;}
               return (h);
       }


int hue_converterR(int hue)
{
    int red = 255;

    if (hue >-1 && hue < 60){
        red = 255;
     //   std::cout << "1 " <<  std::endl;
    }
    else if (hue >59 && hue < 120){
        red = (120-hue)*255/60;
     //   std::cout << "2 " <<  std::endl;
    }
    else if (hue >119 && hue < 180){
        red = 0;
     //   std::cout << "3 " <<  std::endl;
    }
    else if (hue >179 && hue < 240){
        red = 0;
    //    std::cout << "4 " <<  std::endl;
    }
    else if (hue >239 && hue < 300){
        red = 255*(hue-240)/60;
     //   std::cout << "5 " <<  std::endl;
    }
    else if (hue >299 && hue < 360){
        red = 255;
     //   std::cout << "6 " <<  std::endl;
    }
    //std::cout << "red " << red << std::endl;
               return (red);
       }

int hue_converterG(int hue)
{
    int green = 255;

             //What is the hue of each pixel?
    if (hue >-1 && hue < 60){
        green = 255*hue/60;
      //  std::cout << "1 " <<  std::endl;
    }
    else if (hue >59 && hue < 120){
        green = 255;
     //   std::cout << "2 " <<  std::endl;
    }
    else if (hue >119 && hue < 180){
        green = 255;
     //   std::cout << "3 " <<  std::endl;
    }
    else if (hue >179 && hue < 240){
        green = (240-hue)*255/60;
     //   std::cout << "4 " <<  std::endl;
    }
    else if (hue >239 && hue < 300){
        green = 0;
     //   std::cout << "5 " <<  std::endl;
    }
    else if (hue >299 && hue < 360){
        green = 0;
     //   std::cout << "6 " <<  std::endl;
    }
    //std::cout << "green " << green << std::endl;
               return (green);
       }

int hue_converterB(int hue)
{
    int blue = 255;

    if (hue >-1 && hue < 60){
        blue = 0;
    //    std::cout << "1 " <<  std::endl;
    }
    else if (hue >59 && hue < 120){
        blue = 0;
     //   std::cout << "2 " <<  std::endl;
    }
    else if (hue >119 && hue < 180){
        blue = 255*(hue-120)/60;
     //   std::cout << "3 " <<  std::endl;
    }
    else if (hue >179 && hue < 240){
        blue = 255;
     //   std::cout << "4 " <<  std::endl;
    }
    else if (hue >239 && hue < 300){
        blue = 255;
     //   std::cout << "5 " <<  std::endl;
    }
    else if (hue >299 && hue < 360){
        blue = (360-hue)*255/60;
     //   std::cout << "6 " <<  std::endl;
    }
    //std::cout << "blue " << blue << std::endl;
    return (blue);
       }


int scaled_sat_finder(int red, int green, int blue)
{

             //What is the hue of each pixel?
             int smallestrgb = 0;
             int largestrgb = 0;
             int sat = 255;

                   smallestrgb = red;
                   if (green<smallestrgb)
                   { smallestrgb = green;
                   }
                   if (blue<smallestrgb)
                   { smallestrgb = blue;
                   }

                   largestrgb = red;
                   if (green>largestrgb)
                   { largestrgb = green;
                   }
                   if (blue>largestrgb)
                   { largestrgb = blue;
                   }

                   int delta = largestrgb-smallestrgb;

                   if (largestrgb>0)
                   {sat = delta*255/largestrgb;}
                   else
                   {sat = 255;}

               return (sat);
       }

int scaled_value_finder(int red, int green, int blue)
{

            int value = 0.3*red + 0.59*green + 0.11*blue;

               return (red);
       }


/*

o_CPicture<unsigned char>
*image_broadener(o_CPicture<unsigned char> *input_image, o_CPicture<unsigned char> *output_image, int size_x, int size_y, int broaden, int threshold, bool black_white)
{

      for(int xm = broaden;xm<size_x-broaden;xm++)
      {
         for(int ym = broaden;ym<size_y-broaden;ym++)
         {

             int output10 = 0;

             for(int i=0; i<2*broaden+1;i++)
             {
                for (int j =0;j<2*broaden+1;j++)
                {
                    output10 += input_image->GetPixel(xm+i-broaden,ym+j-broaden);
                }
             }
             if(black_white)
             {
                 if (output10/((2*broaden+1)*(2*broaden+1)) < threshold)
                 {output_image->SetPixel(0, xm, ym);}
                 else
                 {output_image->SetPixel(255, xm, ym);}
             }
             else
             {
                 if (output10/((2*broaden+1)*(2*broaden+1)) < threshold)
                 {output_image->SetPixel(255, xm, ym);}
                 else
                 {output_image->SetPixel(0, xm, ym);}
             }
          }
       }
      return (output_image);
  }


o_CPicture<unsigned char>
*image_blurrer(o_CPicture<unsigned char> *input_image, o_CPicture<unsigned char> *output_image, int size_x, int size_y, int broaden)
{

      for(int xm = broaden;xm<size_x-broaden;xm++)
      {
         for(int ym = broaden;ym<size_y-broaden;ym++)
         {

             int output10 = 0;

             for(int i=0; i<2*broaden+1;i++)
             {
                for (int j =0;j<2*broaden+1;j++)
                {
                    output10 += input_image->GetPixel(xm+i-broaden,ym+j-broaden);
                }
             }


output_image->SetPixel(output10/((2*broaden+1)*(2*broaden+1)), xm, ym);}

             }

      return (output_image);
  }



o_CPicture<unsigned char>
*mask_differentiator(o_CPicture<unsigned char> *input_image, o_CPicture<unsigned char> *output_image, int size_x, int size_y, int broadener)
{

  // std::cout << "size x: " << size_x << " size y: " << size_y << std::endl;
   int filter[9] = {-1, 0, 1,
                    -1, 0, 1,
                    -1, 0, 1};

   int smallest = 0;
   int largest = 0;
   int *temparray = new int[size_x*size_y];
   int *temparray2 = new int[size_x*size_y];
   int *temparray3 = new int[size_x*size_y];
   int *temparray5 = new int[size_x*size_y];


   for (int y=0; y<size_y; y++)
   {
       for (int x=0; x<size_x; x++)
       {
        temparray[x+y*size_x] = 0;
        temparray2[x+y*size_x] = 0;
        temparray3[x+y*size_x] = 0;
        temparray5[x+y*size_x] = 0;
       }
   }
 //  std::cout << "ok here 1" << std::endl;
   for (int y=1; y<size_y-1; y++)
   {
       for (int x=1; x<size_x-1; x++)
       {
   //      std::cout << "x: " << x << "  y: " << y << std::endl;
         int output = 0;
         for(int i=0; i<3;i++)
         {
            for (int j =0;j<3;j++)
            {
          //      std::cout << "ok here 1a" << std::endl;
               output += input_image->GetPixel(x+i-1, y+j-1)*filter[(i)+((j)*3)];
            }
         }

         temparray[x+y*size_x] = output;

         if (smallest > output)
         {
           smallest = output;
         }
         if (largest < output)
         {
           largest = output;
         }
       }
   }
//   std::cout << "ok here 2" << std::endl;

   //normalisation
   for (int y=1; y<size_y-1; y++)
   {
       for (int x=1; x<size_x-1; x++)
       {
         bool p = true;
         int output = temparray[x+(y*size_x)];

         output -= smallest;
         if ((float(-smallest+largest))*255.0 !=0)
         {
         output = int((float(output) / float(-smallest+largest))*255.0);

         }
         else
         {  output = 0;}

       }
   }

//   std::cout << "ok here 3" << std::endl;

//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////

//vertical differential (horizontal edge finder)

   int filter2[9] = {-1, -1, -1,
                     0, 0, 0,
                     1, 1, 1};


   int smallest2 = 0;
   int largest2 = 0;
   int smallest3 = 0;
   int largest3 = 0;
   int smallest4 = 0;
   int largest4 = 0;

   for (int y=1; y<size_y-1; y++)
   {


       for (int x=1; x<size_x-1; x++)
       {


         int output2 = 0;
         for(int i=0; i<3;i++)
         {



            for (int j =0;j<3;j++)
            {


               output2 += input_image->GetPixel(x+i-1, y+j-1)*filter2[(i)+((j)*3)];
     //          printf("Output2: %d\n",output2);
            }
         }

         temparray2[x+(y*size_x)] = output2;

         if (smallest2 > output2)
         {
           smallest2 = output2;
         }
         if (largest2 < output2)
         {
           largest2 = output2;
         }
       }
   }


//normalisation
   for (int y=1; y<size_y-1; y++)
   {
       for (int x=1; x<size_x-1; x++)
       {
         bool p = true;
         int output2 = temparray2[x+(y*size_x)];
         //if (output < 0){p = true;}
         output2 -= smallest2;
         output2 = int((float(output2) / float(-smallest2+largest2))*255.0);

       }
   }

   //////////////////////////////////////////////////////////////////////

   //Grad(x,y) which combines the horizontal and vertical edge finders above


   //the bit that does grad(x,y)
    for (int y=1; y<size_y-1; y++)
   {
       for (int x=1; x<size_x-1; x++)
       {
            {
                bool p = true;
                int  output = temparray[x+(y*size_x)];
                int  output2 = temparray2[x+(y*size_x)];
                int  output3 = 1-sqrt(output*output + output2*output2);
               //

            temparray3[x+(y*size_x)] = output3;

            if (smallest3 > output3)
            {
            smallest3 = output3;
            }
            if (largest3 < output3)
            {
            largest3 = output3;
            }
        }

       }
   }

//normalisation
   for (int y=1; y<size_y-1; y++)
   {
       for (int x=1; x<size_x-1; x++)
       {
         bool p = true;
         int output3 = temparray3[x+(y*size_x)];
         //if (output < 0){p = true;}
         output3 -= smallest3;
         output3 = int((float(output3) / float(-smallest3+largest3))*255.0);
         temparray5[x+(y*size_x)] = output3;
     }
   }

   for (int y=broadener; y<size_y-broadener; y++)
   {
       for (int x=broadener; x<size_x-broadener; x++)
       {
         int output10 = 0;
         for(int i=0; i<(2*broadener+1);i++)
         {
            for (int j =0;j<(2*broadener+1);j++)
            {
               output10 += temparray5[(x+i-broadener)+((y*size_x)+j-broadener)];
            }
         }
         if (output10/25 < 200)
         {output10 = 0;}
         else
         {output10 = 255;}

         // this bit takes the edges and emphasises them; makes them really thick

         output_image->SetPixel(output10, x,y);

       }
   }

   delete temparray;
   delete temparray2;
   delete temparray3;
   delete temparray5;

   return (output_image);
}

*/

/*int ltest_houghTransform( _test_houghTransform_parameter &para)
{
  int i, x, y;

  o_RGBPicture<unsigned char> in;

  printf("Reading input...\n");
  bbcvp::ReadImage(in, para.i);

  printf("dims are %dx%d with %d components\n",in.GetNx(), in.GetNy(), in.GetNz());

  int xinc = in.GetElementPtr(1,0,0) - in.GetElementPtr(0,0,0);
  int yinc = in.GetElementPtr(0,1,0) - in.GetElementPtr(0,0,0);

  printf("xinc = %d   yinc= %d\n",xinc, yinc);

  HoughTransform *myHough = new HoughTransform(in.GetNx(), in.GetNy(), 1, 1, 1);

  myHough->setSearchDist(720, 360);
  myHough->setSpreadDist(1, 1);

  // add points in image from green channel
  for (int y=0; y<in.GetNy(); y++)
    {
      unsigned char *thisLine = in.GetElementPtr(0,0,1) + y * yinc;
      for (int x=0; x<in.GetNx(); x++)
        {
            if (thisLine[x*xinc] > 0)
            {
                float xx = x;
                float yy = y;
                float weight = (float)thisLine[x*xinc] / 255.0;
                myHough->addPoints(&xx, &yy, &weight);
            }
        }
    }

  double x0, y0 = 0;
  double x1, y1 = 10;
  float foundWeight;

  foundWeight = myHough->getLine(x0, y0, x1, y1, &x0, &y0, &x1, &y1);

  printf("Found a line with a weight of %6.2f\n", foundWeight);

  FittedLine foundLine(x0, y0, x1, y1);
  foundLine.drawLine(in.GetElementPtr(0,0,1), xinc, yinc, in.GetNx(), in.GetNy(), 255, 2);

  printf("Writing output...\n");
  bbcvp::WriteImage(in, para.o);
*/
 /* if (para.hout_flag)
  {
      int xl, yl;
      myHough->getHoughSpaceSize(&xl, &yl);
      o_RGBPicture<unsigned char> houghOut(xl, yl);
      int xi = houghOut.GetElementPtr(1,0,0) - houghOut.GetElementPtr(0,0,0);
      int yi = houghOut.GetElementPtr(0,1,0) - houghOut.GetElementPtr(0,0,0);
      myHough->getHoughSpace(houghOut.GetElementPtr(0,0,0), xi, yi, 100);  // write to R
      myHough->getHoughSpace(houghOut.GetElementPtr(0,0,1), xi, yi, 100);  // and to G
      myHough->getHoughSpace(houghOut.GetElementPtr(0,0,2), xi, yi, 100);  // and to B
      printf("Writing image holding Hough array...\n");
      bbcvp::WriteImage(houghOut, para.hout);
  }

  return (0);

}

*/


/*
      int pixelvalue = 0;
      int whiteonset_x = 0;
      int whiteoffset_x = 0;
      int whiteonset_y = 0;
      int whiteoffset_y = 0;
      int average_width = 0;
      int width_ticker = 0;

      for(int ym = 1;ym<lenY-1;ym++)
         {
         for(int xm = 1;xm<lenX-1;xm++)
            {
             if (arsemask5->GetPixel(xm, ym) == 255)
             {
                 arsemask6->SetPixel(128, xm, ym);
             }
             else
                 arsemask6->SetPixel(255, xm, ym);

             if (arsemask5->GetPixel(xm, ym) == 255 && arsemask5->GetPixel(xm, ym) - pixelvalue == 255 && xm > lenX/6 && xm < 5*lenX/6 )
             {
                 whiteonset_x = xm;
                 pixelvalue = 255;
             }
             if (arsemask5->GetPixel(xm, ym) == 0 && arsemask5->GetPixel(xm, ym) - pixelvalue == -255 && xm > lenX/6 && xm < 5*lenX/6)
             {
                 width_ticker += 1;
                 whiteoffset_x = xm;

                 if (width_ticker <100)
                 {
                     average_width = (average_width*(width_ticker-1)+(whiteoffset_x - whiteonset_x))/width_ticker;
                 }

                 pixelvalue = 0;
                 if ( (whiteoffset_x - whiteonset_x) > 3*average_width && width_ticker > 3)
                 {
                 arsemask6->SetPixel(0, whiteonset_x + (whiteoffset_x - whiteonset_x)/4 , ym);
                 arsemask6->SetPixel(0, whiteonset_x + (whiteoffset_x - whiteonset_x)/4 + 1 , ym);
                 arsemask6->SetPixel(0, whiteonset_x + (whiteoffset_x - whiteonset_x)/4 - 1 , ym);
                 arsemask6->SetPixel(0, whiteonset_x + 3*(whiteoffset_x - whiteonset_x)/4 , ym);
                 arsemask6->SetPixel(0, whiteonset_x + 3*(whiteoffset_x - whiteonset_x)/4 + 1 , ym);
                 arsemask6->SetPixel(0, whiteonset_x + 3*(whiteoffset_x - whiteonset_x)/4 - 1 , ym);
                 }
                 else
                 {
                 arsemask6->SetPixel(0, (whiteonset_x + whiteoffset_x)/2 , ym);
                 arsemask6->SetPixel(0, (whiteonset_x + whiteoffset_x)/2 + 1 , ym);
                 arsemask6->SetPixel(0, (whiteonset_x + whiteoffset_x)/2 - 1 , ym);
                 }
             }
             else
             {}
         }
     }

      pixelvalue = 0;

         for(int xm = 1;xm<lenX-1;xm++)
            {
             for(int ym = 1;ym<lenY-1;ym++)
                {
             if (arsemask5->GetPixel(xm, ym) == 255)
             {
                 arsemask7->SetPixel(128+satmask->GetPixel(xm, ym)/2, xm, ym);
             }
             else
                 arsemask7->SetPixel(255, xm, ym);

             if (arsemask5->GetPixel(xm, ym) == 255 && arsemask5->GetPixel(xm, ym) - pixelvalue == 255 && ym > lenY/6 && ym < 5*lenY/6 )
             {
                 whiteonset_y = ym;
                 pixelvalue = 255;
             }
             if ((arsemask5->GetPixel(xm, ym) == 0 && arsemask5->GetPixel(xm, ym) - pixelvalue == -255 && ym > lenY/6 && ym < 5*lenY/6))
             {
                 whiteoffset_y = ym;
                 pixelvalue = 0;

                 if ( (whiteoffset_y - whiteonset_y) > 3*average_width && width_ticker > 3)
                 {
                 arsemask7->SetPixel(0, xm, whiteonset_y + (whiteoffset_y - whiteonset_y)/4);
                 arsemask7->SetPixel(0, xm, whiteonset_y + (whiteoffset_y - whiteonset_y)/4 + 1);
                 arsemask7->SetPixel(0, xm, whiteonset_y + (whiteoffset_y - whiteonset_y)/4 - 1);
                 arsemask7->SetPixel(0, xm, whiteonset_y + 3*(whiteoffset_y - whiteonset_y)/4);
                 arsemask7->SetPixel(0, xm, whiteonset_y + 3*(whiteoffset_y - whiteonset_y)/4 + 1);
                 arsemask7->SetPixel(0, xm, whiteonset_y + 3*(whiteoffset_y - whiteonset_y)/4 - 1);
                 }
                 else
                 {
                 arsemask7->SetPixel(0, xm, (whiteonset_y + whiteoffset_y)/2);
                 arsemask7->SetPixel(0, xm, (whiteonset_y + whiteoffset_y)/2 + 1);
                 arsemask7->SetPixel(0, xm, (whiteonset_y + whiteoffset_y)/2 - 1);
                 }
             }
             else
             {}
             if (arsemask6->GetPixel(xm, ym) == 0)
             {
                 arsemask7->SetPixel(0, xm , ym);
             }
         }
     }
*/

