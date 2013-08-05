
//
//	This is a example how to use the command parser
//	O. Grau, Nov-2001

#include	<stdlib.h>
#include	<string.h>
#include	$(VPLIBSHOME)/VPbase/Picture.h
#include	"puttCalc.h"
#include	"PicReadFileSeq.h"
#include	"WriteImage.h"
#include        "Region.h"
#include        "objectDetect.cc"

//#include	"Deinterlace.h"

// Marck


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


int scaled_sat_finder(int red, int green, int blue)
{

             //What is the hue of each pixel?
             int smallestrgb = 0;
             int largestrgb = 0;
             int sat = 0;

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
                   {sat = delta*256/largestrgb;}
                   else
                   {sat = -1;}

               return (sat);
       }


int
ltestarg( _puttCalc_parameter &para)
{
	vpcoreio::PicReadFileSeq	picseq;
	bbcvp::s64		frame_count;
	bbcvp::ErrCode	err;
	char	buf[1000];
        int ball_pixel_x = 0;
        int ball_pixel_y = 0;


	// pass the arguments through
	if( para.fn_flag)
	   sprintf(buf,"-fn %s -start %d -end %d -inc %d -fmt %d -dx %d -dy %d -filt %d",
		para.fn,para.start,para.end, para.inc,
		para.fmt, para.dx, para.dy ,para.filt ); //, para.deint);
	else
	   sprintf(buf,"-patt %s -start %d -end %d -inc %d -fmt %d -dx %d -dy %d -filt %d",
		para.patt,para.start,para.end, para.inc,
		para.fmt, para.dx, para.dy, para.filt );
	if(para.cam_flag) sprintf(buf+strlen(buf)," -cam %s",para.cam);
	else if(para.campatt_flag) sprintf(buf+strlen(buf)," -campatt %s",para.campatt);

	// print out current argument list
	std::cout<<"Arg: "<<buf<<"\n";

	//
	// Open image sequence
	//
	err = picseq.Open( buf );


	const char *tmp;
	tmp = err==bbcvp::OK?"OK!":"Error!";
	std::cout<<"Err: "<<tmp<<"; code="<<err<<std::endl;
	if (err!=bbcvp::OK) { return err; }
	
	std::cout << "printfBuf: " << buf << std::endl;
	std::cout << "printfErr: " << err << std::endl;

	//
	//	Loop over all selected frames of sequence
	//
	//bbcvp::TimeCode	lastcam_tc;
	//lastcam_tc = picseq.GetCamera().GetTimeStamp();
	
	const int seq_len = picseq.GetNumberOfPictures();
	int frames_to_process = seq_len / para.inc;
	if (seq_len % para.inc > 0)
	  ++frames_to_process;
	std::cout<<"Processing every "<<para.inc<<". frame between ["<<
		para.start<<";"<<para.end<<"] ==> "<<
		frames_to_process<<" frames in total."<<std::endl;


        int frameNumber[frames_to_process];
        double ballDistance[frames_to_process];

        for(frame_count=0; frame_count<frames_to_process; frame_count++)
        {
            frameNumber[frame_count] = 0;
            ballDistance[frame_count] = 0;
        }

	// TODO: add some parallelisation here?
	for( frame_count=0; ; ++frame_count ) {
		err = picseq.LoadNextPicture();
		if(err!=bbcvp::OK) {
			// if end of sequence is reached, program doesn't return error!
			if (frame_count == frames_to_process) {
				std::cout<<"Done! processed frames: "<<frame_count<<std::endl;
				err = bbcvp::OK;
			} else {
				std::cerr<<"Err: picseq.LoadNextPicture() returned: "<<
					err<<", frame_count="<<frame_count<<
					", frames_to_process="<<frames_to_process<<
					", seq_len="<<seq_len<<std::endl;
			}
			break;
		}




                bbcvp::CPicture<unsigned char> picBuf = picseq.GetPicture();
                bbcvp::CPicture<unsigned char> golfmask(picBuf.GetNx(), picBuf.GetNy(),1);
                bbcvp::CPicture<unsigned char> golfmap(picBuf.GetNx(), picBuf.GetNy(),1);

                int huemax = 0;
                int huemin = 0;
                int greenhue = 0;
                int huehistogram[360];
                int totalnumberofpixels = picBuf.GetNx()*picBuf.GetNy();
                int pixeldataarray[totalnumberofpixels];
                int pixeldataarrayRGB[totalnumberofpixels];
                int temparray2[totalnumberofpixels];

                for (int k=0; k<360; k++)
                {
                  huehistogram[k] = 0;
                }

                // pixeldataarray is and array which can store data about each pixel
                for (int k=0; k<totalnumberofpixels; k++)
                {
                pixeldataarray[k] = 0;
                pixeldataarrayRGB[k] = 0;
                temparray2[k] = 0;
                }
		
                for (int myPixel_x = 0 ; myPixel_x < picBuf.GetNx(); myPixel_x++)
                {
                    for (int myPixel_y = 0 ; myPixel_y < picBuf.GetNy(); myPixel_y++)
                    {
                        unsigned char myPixel_R = picBuf.GetPixel(myPixel_x,myPixel_y, 0);
                        int myInt_R = static_cast<int>(myPixel_R);
                        unsigned char myPixel_G = picBuf.GetPixel(myPixel_x,myPixel_y, 1);
                        int myInt_G = static_cast<int>(myPixel_G);
                        unsigned char myPixel_B = picBuf.GetPixel(myPixel_x,myPixel_y, 2);
                        int myInt_B = static_cast<int>(myPixel_B);

                        greenhue = hue_finder(myInt_R, myInt_G, myInt_B);
                        pixeldataarray[myPixel_x+myPixel_y*picBuf.GetNx()]=greenhue;
                        pixeldataarrayRGB[myPixel_x+myPixel_y*picBuf.GetNx()]=myInt_R+myInt_G+myInt_B;
                        huehistogram[greenhue] += 1;
                    }
                }

                int largesthue = 0;
                int largesthuenumber = 0;

                for (int k=0; k<360; k++)
                {
                    if (largesthue < huehistogram[k])
                    {
                        largesthue = huehistogram[k];
                        largesthuenumber = k;
                    }
                }

                for (int k=largesthuenumber; k<largesthuenumber+30; k++)
                {
                    if (huehistogram[k]>huehistogram[k+1]+100)
                    {
                        huemax = k;
                    }
                }

                for (int k=largesthuenumber-30; k<largesthuenumber; k++)
                {
                    if (huehistogram[k]+100>huehistogram[k+1])
                    {
                        huemin = k;
                    }
                }

                //**********mask differentiator************//
                  int filter2[9] = {-1, 0, 1,
                                    -1, 0, 1,
                                    -1, 0, 1};

                  int smallest2 = 0;
                  int largest2 = 0;

                  for (int y=1; y<picBuf.GetNy()-1; y++)
                  {
                      for (int x=1; x<picBuf.GetNx()-1; x++)
                      {
                          int output2 = 0;
                          for(int i=0; i<3;i++)
                          {
                              for (int j =0;j<3;j++)
                              {
                                  output2 += picBuf.GetPixel(x+i-1, y+j-1,1)*filter2[(i)+((j)*3)];
                              }
                          }

                          temparray2[x+(y*picBuf.GetNx())] = output2;

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
                  for (int y=1; y<picBuf.GetNy()-1; y++)
                  {
                      for (int x=1; x<picBuf.GetNx()-1; x++)
                      {
                          int output2 = temparray2[x+(y*picBuf.GetNx())];
                          output2 -= smallest2;
                          output2 = int((float(output2) / float(-smallest2+largest2))*255.0);
                          golfmap.SetPixel(output2, x, y);
                      }
                  }

                //int brightness_differential = 300;

                for (int myPixel_x = 0 ; myPixel_x < picBuf.GetNx(); myPixel_x++)
                {
                    for (int myPixel_y = 0 ; myPixel_y < picBuf.GetNy(); myPixel_y++)
                    {
                        if(pixeldataarray[myPixel_x+myPixel_y*picBuf.GetNx()]<huemin - 2
                                || pixeldataarray[myPixel_x+myPixel_y*picBuf.GetNx()]> huemax + 2
                                || golfmap.GetPixel(myPixel_x, myPixel_y) > 200
                                || golfmap.GetPixel(myPixel_x, myPixel_y) < 100
                                )
                        {
                            golfmask.SetPixel(255, myPixel_x, myPixel_y);
                        }
                        else
                        {
                            golfmask.SetPixel(0, myPixel_x, myPixel_y);
                        }

                    }
                }



                int whiteness_counter[picBuf.GetNx()];
                int max_whiteness = 0;
                int flag_line = 0;
                int min_flag = 0;
                int max_flag = 0;

                for (int myPixel_x = 3 ; myPixel_x < picBuf.GetNx()-3; myPixel_x++)
                {
                    int vertical_white = 0;

                    for (int myPixel_y = 3 ; myPixel_y < picBuf.GetNy()-3; myPixel_y++)
                    {
                        if(golfmask.GetPixel(myPixel_x, myPixel_y) == 255)
                            vertical_white +=1;
                    }
                    whiteness_counter[myPixel_x] = vertical_white;
                    // std::cout << "WHITENESS COUNTER [" << myPixel_x << "] = " << whiteness_counter[myPixel_x] << std::endl;
                    if (vertical_white > max_whiteness)
                    {
                        max_whiteness = vertical_white;
                        flag_line = myPixel_x;
                    }
                }

                if (flag_line > 10 && flag_line < picBuf.GetNx()-11)
                {
                    for (int myPixel_x = flag_line-9 ; myPixel_x < flag_line+10; myPixel_x++)
                    {
                        for (int myPixel_y = 3 ; myPixel_y < picBuf.GetNy()-3; myPixel_y++)
                        {
                            unsigned char myPixel_R = picBuf.GetPixel(myPixel_x,myPixel_y, 0);
                            int myInt_R = static_cast<int>(myPixel_R);
                            unsigned char myPixel_G = picBuf.GetPixel(myPixel_x,myPixel_y, 1);
                            int myInt_G = static_cast<int>(myPixel_G);
                            unsigned char myPixel_B = picBuf.GetPixel(myPixel_x,myPixel_y, 2);
                            int myInt_B = static_cast<int>(myPixel_B);

                            if (golfmask.GetPixel(myPixel_x, myPixel_y) == 255 && (max_flag == 0 || max_flag > myPixel_y)
                                    && hue_finder(myInt_R, myInt_G, myInt_B) < 70 && hue_finder(myInt_R, myInt_G, myInt_B) > 45
                                    )
                            {
                                max_flag = myPixel_y;
                            }
                            if (golfmask.GetPixel(myPixel_x, myPixel_y) == 255 && myPixel_y > min_flag
                                    && (hue_finder(myInt_R, myInt_G, myInt_B) > 70 || hue_finder(myInt_R, myInt_G, myInt_B) < 45)
                                    )
                            {
                                min_flag = myPixel_y;
                            }
                        }

                    }
                }

                #define MAX_REGION 10000
                bbcvp::Region *ballregion = new bbcvp::Region[MAX_REGION];

                int indexes[MAX_REGION];

                Point searchRegionPoint1a(0,0);
                Point searchRegionPoint2a(picBuf.GetNx()-1,picBuf.GetNy()-1);

               //store the number of regions found
                int number_regions;

                objectDetection(golfmask.GetElementPtr(0,0,0),
                                golfmap.GetElementPtr(0,0,0),
                                golfmap.GetNx(),
                                golfmap.GetNy(),
                                ballregion,
                                &number_regions,
                                indexes,
                                searchRegionPoint1a,
                                searchRegionPoint2a,
                                false);

                std::cout << "Are we getting this far?" << std::endl;

                double distanceToHole = 0;

                int tot_pix_in_regions=0;
                int zone_ticker = 0;
                printf("Region  top-left  bottom-right  no. of pixels   x range on top row \n");
                for (int ind=0; ind<number_regions; ind++)
                {
                    int i = indexes[ind];
                    int possible_ball_x = 0;
                    int possible_ball_y = 0;
                    int x_difference = 0;
                    int y_difference = 0;
                    int ball_centre_x = (ballregion[i].getMaxX()+ballregion[i].getMinX())/2;
                    int ball_centre_y = (ballregion[i].getMaxY()+ballregion[i].getMinY())/2;
                    if (ballregion[i].getMinX()<ballregion[i].getMaxX() &&
                            ballregion[i].getNumberPixels()< 100 &&
                            ballregion[i].getNumberPixels()> 10 &&
                            (ball_centre_x + 6 < flag_line ||
                             ball_centre_x - 6 > flag_line ||
                             ball_centre_y + 6 < max_flag ||
                             ball_centre_y - 6 > min_flag)  &&
                            number_regions < 10 )

                    {
                        printf("%3d     (%3d,%3d)   (%3d,%3d)   %6d            %3d->%3d\n", i,
                               ballregion[i].getMinX(), ballregion[i].getMinY(),
                               ballregion[i].getMaxX(), ballregion[i].getMaxY(),
                               ballregion[i].getNumberPixels(),
                               ballregion[i].getTopLineMinX(), ballregion[i].getTopLineMaxX() );
                        tot_pix_in_regions += ballregion[i].getNumberPixels();
                        zone_ticker +=1;
                        possible_ball_x = ball_centre_x;
                        possible_ball_y = ball_centre_y;
                        x_difference = (possible_ball_x-ball_pixel_x)*(possible_ball_x-ball_pixel_x);
                        y_difference = (possible_ball_y-ball_pixel_y)*(possible_ball_y-ball_pixel_y);
                        std::cout << possible_ball_x << "," << possible_ball_y << std::endl;
                        std::cout << "x difference " << x_difference << "   y difference " << y_difference << std::endl;

                        if ((ball_pixel_x == 0 && ball_pixel_y == 0 && zone_ticker ==1) ||
                            (x_difference+y_difference < 1000))
                        {
                            ball_pixel_x = possible_ball_x;
                            ball_pixel_y = possible_ball_y;
                            std::cout << "BALL X " << ball_pixel_x << std::endl;
                            std::cout << "BALL Y " << ball_pixel_y << std::endl;
                        }


                        double distanceFromFootOfFlag = 95;
                        double heightOfFlag = 6;
                        double heightOfCameraOnTower = 25;

                        double topFlagRatio = (double)distanceFromFootOfFlag/(double)(heightOfCameraOnTower-heightOfFlag);
                        double bottomFlagRatio = (double)distanceFromFootOfFlag/(double)heightOfCameraOnTower;
                        double screenRatio = (double)((double)min_flag-(double)ball_pixel_y)/(double)((double)min_flag - (double)max_flag);

                        double thetaFlag = (atan(topFlagRatio) - atan(bottomFlagRatio));
                        double scaleFactor = (double)((double)min_flag-(double)max_flag)/((double)heightOfFlag*sin(3.142/2-atan(1/bottomFlagRatio)));
                        double thetaBall =  screenRatio*thetaFlag;
                        double thetaFromBallToTower = atan(1/bottomFlagRatio) - thetaBall;
                        double distanceFromTowerToBall = heightOfCameraOnTower/tan(thetaFromBallToTower);
                        double distanceToBallFromTowerTop = sqrt(distanceFromTowerToBall*distanceFromTowerToBall+heightOfCameraOnTower*heightOfCameraOnTower);
                        double pixelDistanceFromFlagX = ball_pixel_x - flag_line;
                        double distanceFromTowerTopToFlag = sqrt(distanceFromFootOfFlag*distanceFromFootOfFlag+heightOfCameraOnTower*heightOfCameraOnTower);
                        double depthScaleFactor = scaleFactor*distanceFromTowerToBall/distanceFromFootOfFlag;
                        double distanceRightOfHole = distanceToBallFromTowerTop*pixelDistanceFromFlagX/depthScaleFactor/distanceFromTowerTopToFlag;

/*
                        std::cout << "topFlagRatio: " << topFlagRatio << std::endl;
                        std::cout << "bottomFlagRatio: " << bottomFlagRatio << std::endl;
                        std::cout << "screenRatio: " << screenRatio << std::endl;
                        std::cout << "thetaFlag: " << thetaFlag << std::endl;
                        std::cout << "scaleFactor: " << scaleFactor << std::endl;
                        std::cout << "thetaBall: " << thetaBall << std::endl;
                        std::cout << "thetaFromBallToTower: " << thetaFromBallToTower << std::endl;
                        std::cout << "distanceFromTowerToBall: " << distanceFromTowerToBall << std::endl;
                        std::cout << "distanceToBallFromTowerTop: " << distanceToBallFromTowerTop << std::endl;
                        std::cout << "pixelDistanceFromFlagX : " << pixelDistanceFromFlagX  << std::endl;
                        std::cout << "distanceFromTowerTopToFlag: " << distanceFromTowerTopToFlag << std::endl;
                        std::cout << "depthScaleFactor: " << depthScaleFactor << std::endl;
                        std::cout << "distanceRightOfHole: " << distanceRightOfHole << std::endl;
*/
                        double distanceShortOfHole = distanceFromTowerToBall-distanceFromFootOfFlag;
                      //  std::cout << "distanceShortOfHole: " << distanceShortOfHole << std::endl;

                        distanceToHole =  sqrt(distanceShortOfHole*distanceShortOfHole+distanceRightOfHole*distanceRightOfHole);
                        std::cout << "DISTANCE TO HOLE: " << distanceToHole << std::endl;


                    }
                }
                printf("Total pixels in all allocated regions = %d\n",tot_pix_in_regions);

                frameNumber[frame_count] = (int)frame_count + para.start;
                ballDistance[frame_count] = distanceToHole;


                for (int myPixel_y = 0 ; myPixel_y < picBuf.GetNy(); myPixel_y++)
                {
                                                 golfmask.SetPixel(128, flag_line, myPixel_y);
             if(flag_line < picBuf.GetNx())      golfmask.SetPixel(128, flag_line+1, myPixel_y);
             if(flag_line > 0)                   golfmask.SetPixel(128, flag_line-1, myPixel_y);
             if(ball_pixel_x != 0)               golfmask.SetPixel(128, ball_pixel_x, myPixel_y);
                }

                for (int myPixel_x = 0 ; myPixel_x < picBuf.GetNx(); myPixel_x++)
                {
                                                 golfmask.SetPixel(128, myPixel_x, min_flag);
              if(min_flag < picBuf.GetNx())      golfmask.SetPixel(128, myPixel_x, min_flag+1);
              if(min_flag > 0)                   golfmask.SetPixel(128, myPixel_x, min_flag-1);
                                                 golfmask.SetPixel(128, myPixel_x, max_flag);
              if(max_flag < picBuf.GetNx())      golfmask.SetPixel(128, myPixel_x, max_flag+1);
              if(max_flag > 0)                   golfmask.SetPixel(128, myPixel_x, max_flag-1);
              if(ball_pixel_y != 0)              golfmask.SetPixel(128, myPixel_x, ball_pixel_y);
                }


                sprintf(buf, "fowler/golfmask/%04d.tiff", (int)frame_count + para.start);
                vpcoreio::WriteImage(golfmask, buf);
                sprintf(buf, "fowler/golfmap/%04d.tiff", (int)frame_count + para.start);
                vpcoreio::WriteImage(golfmap, buf);

                std::cout << "FLAG LINE: " << flag_line << std::endl;
                std::cout << "FLAG MIN: " << min_flag << std::endl;
                std::cout << "FLAG MAX: " << max_flag << std::endl;
                std::cout << "DISTANCE SHORT OF HOLE: " << max_flag << std::endl;
		
		std::cout << "printfBuf: " << buf << std::endl;
		std::cout << "printfErr: " << err << std::endl;
                //std::cout << "print picbuf pixel" << myPixel << std::endl;
                //std::cout << "print int " << myInt << std::endl;

                if(frame_count==frames_to_process-1)
                {
                    for(int i = 0; i < frames_to_process; i++)
                    {
                        std::cout << "Frame: " << frameNumber[i] << ",   Distance To Hole: " << ballDistance[i] << std::endl;
                    }
                }
	
		
		
		
		int	oidx = picseq.GetCurrentTC().GetLin();
		std::cout<<"Got picture "<<oidx<<"\n";

		/* write out images
		*/
		/*if(para.outpatt_flag) {
			char tbuf[1000];
			if(para.deint==2) oidx *= 2;
			sprintf(tbuf,para.outpatt,oidx );
			std::cout<<"Write image "<<tbuf<<" "<<std::flush;
			if(para.deint) {
				bbcvp::CPicture<unsigned char> depic;
				int	field = 0;
				bbcvp::Deinterlace( picseq.GetPicture(), depic, field, para.filt );
				vpcoreio::WriteImage( depic,tbuf);
				if(para.deint==2) {
					sprintf(tbuf,para.outpatt,oidx+1 );
					bbcvp::Deinterlace( picseq.GetPicture(), depic, field+1, para.filt );
					std::cout<<"Write image "<<tbuf<<" "<<std::flush;
					vpcoreio::WriteImage( depic,tbuf);
				}
			} else {
				vpcoreio::WriteImage(picseq.GetPicture(),tbuf);
			//}
			std::cout<<"ok!\n";
		}

		// check if new camera parameter have arrived
		if(lastcam_tc != picseq.GetCamera().GetTimeStamp()) {
			std::cout<<"Got new camera parameter "<<picseq.GetCamera().GetTimeStamp().GetLin() <<"\n";
			lastcam_tc = picseq.GetCamera().GetTimeStamp();
		}*/
		std::cout<<"...\n";
	}
	// Close file sequence and return result
	picseq.Close();
	return err;
}



int
main(int argc, char *argv[] )
{
	_puttCalc_parameter	para;

	if(ParseArg_puttCalc(argc,argv,para)<1) return 0;
	para.Print(stdout);
	bbcvp::init_oGeM(true);
	bbcvp::SetExitError(2);
	return ltestarg(para);
}



