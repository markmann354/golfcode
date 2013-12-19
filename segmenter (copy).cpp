bool segmenter(bbcvp::CPicture<unsigned char> picBuf,
               double golfscreen_x[],
               double golfscreen_y[],
               bool isObject[],
               char buf,
               int start,
               int frame_count,
               int end,
               bbcvp::CPicture<unsigned char> *fieldmap,
               bbcvp::CPicture<unsigned char> *golfmask,
               bbcvp::CPicture<unsigned char> *golfmap,
               bbcvp::CPicture<unsigned char> *golfsat,
               bbcvp::CPicture<unsigned char> *flagmap,
               bbcvp::CPicture<unsigned char> *flagspot,
               bbcvp::CPicture<unsigned char> *golfdiff,
               bbcvp::CPicture<unsigned char> *valuemap,
               bbcvp::CPicture<unsigned char> *satmask,
               bbcvp::CPicture<unsigned char> *huemap,
               bbcvp::CPicture<unsigned char> *output
               ){

    int screenWidth = 0;
    int screenHeight = 0;
    int flag_line = 0;
    int max_flag = 0;
    int min_flag = 0;
    int ball_pixel_x = 0;
    int ball_pixel_y = 0;

    int frames_to_process = end-start;

    int frameNumber[frames_to_process];
    double ballDistance[frames_to_process];

    for(frame_count=0; frame_count<frames_to_process; frame_count++)
    {
        frameNumber[frame_count] = 0;
        ballDistance[frame_count] = 0;
    }

    int totalnumberofpixels = picBuf.GetNx()*picBuf.GetNy();
    screenWidth = picBuf.GetNx();
    screenHeight = picBuf.GetNy();

    //bbcvp::CPicture<unsigned char> green = picseq2.GetPicture();


    totalnumberofpixels = screenWidth*screenHeight;

    int huemax = 0;
    int huemin = 0;
    int greenhue = 0;
    int *huehistogram = new int[360];
    int *pixeldataarray = new int[totalnumberofpixels];
    int *pixeldataarrayRGB = new int[totalnumberofpixels];
    int *temparray2 = new int[totalnumberofpixels];
    int *temparray3 = new int[totalnumberofpixels];
    int sat = 0;
    int valueM = 0;

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

    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
        {
            unsigned char myPixel_R = picBuf.GetPixel(myPixel_x,myPixel_y, 0);
            int myInt_R = static_cast<int>(myPixel_R);
            unsigned char myPixel_G = picBuf.GetPixel(myPixel_x,myPixel_y, 1);
            int myInt_G = static_cast<int>(myPixel_G);
            unsigned char myPixel_B = picBuf.GetPixel(myPixel_x,myPixel_y, 2);
            int myInt_B = static_cast<int>(myPixel_B);


            greenhue = hue_finder(myInt_R, myInt_G, myInt_B);
            valueM = scaled_value_finder(myInt_R, myInt_G, myInt_B);
            sat = scaled_sat_finder(myInt_R, myInt_G, myInt_B);
            pixeldataarray[myPixel_x+myPixel_y*screenWidth]=greenhue;
            pixeldataarrayRGB[myPixel_x+myPixel_y*screenWidth]=myInt_R+myInt_G+myInt_B;
            huehistogram[greenhue] += 1;
            golfsat->SetPixel(sat,myPixel_x,myPixel_y);

            if(myPixel_x % 10 == 0 && myPixel_y % 10 == 0){
              //  cout << myPixel_x << "," << myPixel_y << " : " << sat << endl;
            }

            if(sat<60){
                satmask->SetPixel(255,myPixel_x,myPixel_y);
            }
            else{
                satmask->SetPixel(0,myPixel_x,myPixel_y);
            }

            if(valueM < 128){
                valuemap->SetPixel(255,myPixel_x,myPixel_y);
            }
            else{
                valuemap->SetPixel(0,myPixel_x,myPixel_y);
            }

            int colours[3];
            colours[0] = hue_converterR(greenhue);
            colours[1] = hue_converterG(greenhue);
            colours[2] = hue_converterB(greenhue);
            if(greenhue > 330 || greenhue < 30){
                huemap->SetPixel(colours[0],myPixel_x,myPixel_y,0);
                huemap->SetPixel(colours[1],myPixel_x,myPixel_y,1);
                huemap->SetPixel(colours[2],myPixel_x,myPixel_y,2);
                flagspot->SetPixel(255, myPixel_x, myPixel_y);
            }
            else{
                huemap->SetPixel(0,myPixel_x,myPixel_y,0);
                huemap->SetPixel(0,myPixel_x,myPixel_y,1);
                huemap->SetPixel(0,myPixel_x,myPixel_y,2);
                flagspot->SetPixel(0, myPixel_x, myPixel_y);
            }
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
            huemax = 180; //k;
        }
    }

    for (int k=largesthuenumber-30; k<largesthuenumber; k++)
    {
        if (huehistogram[k]+100>huehistogram[k+1])
        {
            huemin = 0;
        }
    }

    //**********mask differentiator************/

    int filter2[9] = {-1, 0, 1,
                      -1, 0, 1,
                      -1, 0, 1};

    int smallest2 = 0;
    int largest2 = 0;
    int smallest3 = 0;
    int largest3 = 0;

    for (int y=1; y<screenHeight-1; y++)
    {
        for (int x=1; x<screenWidth-1; x++)
        {
            int output2 = 0;
            int output3 = 0;

            for(int i=0; i<3;i++)
            {
                for (int j =0;j<3;j++)
                {
                    output2 += golfsat->GetPixel(x+i-1, y+j-1)*filter2[(i)+((j)*3)];
                    output3 += valuemap->GetPixel(x+i-1, y+j-1)*filter2[(i)+((j)*3)];
                }
            }

            temparray2[x+(y*screenWidth)] = output2;
            temparray3[x+(y*screenWidth)] = output3;

            if (smallest2 > output2)
            {
                smallest2 = output2;
            }
            if (largest2 < output2)
            {
                largest2 = output2;
            }
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

    int last_y = 0;
    int lastbutone_y = 0;
    //normalisation
    for (int y=1; y<screenHeight-1; y++)
    {
        for (int x=1; x<screenWidth-1; x++)
        {
            int output2 = temparray2[x+(y*screenWidth)];
            output2 -= smallest2;
            output2 = int((float(output2) / float(-smallest2+largest2))*255.0);

            int output3 = temparray3[x+(y*screenWidth)];
            output3 -= smallest3;
            output3 = int((float(output3) / float(-smallest3+largest3))*255.0);
            golfdiff->SetPixel(output3, x,y);

            if ((output2>160 || output2 < 100) //|| (output3>160 || output3 < 100)
                    || last_y == 255 || lastbutone_y == 255
                    )
            {
                golfmap->SetPixel(255, x, y);
            }
            else {
                golfmap->SetPixel(0, x, y);
            }
            if(golfmap->GetPixel(x,y) == 255 && last_y == 0){last_y = 255; ; lastbutone_y = 0;}
            else if(golfmap->GetPixel(x,y) == 255 && last_y == 255 && lastbutone_y ==0){last_y = 255;  lastbutone_y = 255;}
            else if(golfmap->GetPixel(x,y) == 0 && last_y == 255 && lastbutone_y ==255){last_y = 0;  lastbutone_y = 255;}
            else if(golfmap->GetPixel(x,y) == 0 && last_y == 0 && lastbutone_y ==255){last_y = 0;  lastbutone_y = 0;}
            else{last_y = 0; lastbutone_y = 0;}
        }
    }

    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
        {
            if(pixeldataarray[myPixel_x+myPixel_y*screenWidth]<huemin - 2
                    || pixeldataarray[myPixel_x+myPixel_y*screenWidth]> huemax + 2
                    || golfmap->GetPixel(myPixel_x, myPixel_y) == 255
                    || golfsat->GetPixel(myPixel_x, myPixel_y) < 60
                    )
            {
                golfmask->SetPixel(255, myPixel_x, myPixel_y);
            }
            else
            {
                golfmask->SetPixel(0, myPixel_x, myPixel_y);
            }

        }
    }


    int whiteness_counter[screenWidth];
    int max_whiteness = 0;

    for (int myPixel_x = 3 ; myPixel_x < screenWidth-3; myPixel_x++)
    {
        int vertical_white = 0;

        for (int myPixel_y = 3 ; myPixel_y < screenHeight-3; myPixel_y++)
        {
            if(golfmap->GetPixel(myPixel_x, myPixel_y) == 255)
                vertical_white +=1;
        }
        whiteness_counter[myPixel_x] = vertical_white;
        if (vertical_white > max_whiteness)
        {
            max_whiteness = vertical_white;
            flag_line = myPixel_x;
        }
    }

#define MAX_REGION2 10000
    bbcvp::Region *flagregion = new bbcvp::Region[MAX_REGION2];

    int indexes2[MAX_REGION2];

    Point searchRegionPoint1b(0,0);
    Point searchRegionPoint2b(screenWidth-1,screenHeight-1);

    //store the number of regions found
    int number_regions2;

    objectDetection(flagspot->GetElementPtr(0,0,0),
                    flagmap->GetElementPtr(0,0,0),
                    flagmap->GetNx(),
                    flagmap->GetNy(),
                    flagregion,
                    &number_regions2,
                    indexes2,
                    searchRegionPoint1b,
                    searchRegionPoint2b,
                    false);

    int firstone = 0;
    for (int ind=0; ind<number_regions2; ind++)
    {
        int i = indexes2[ind];

        if (flagregion[i].getMaxX() > flag_line && flagregion[i].getMinX() < flag_line
                && flagregion[i].getMinY() > 40)
        {

            if(firstone == 0){
                max_flag = flagregion[i].getMinY();
                firstone = 1;
            }
        }
    }

    if (flag_line > 10 && flag_line < screenWidth-11)
    {
        for (int myPixel_x = flag_line-9 ; myPixel_x < flag_line+10; myPixel_x++)
        {
            for (int myPixel_y = 3 ; myPixel_y < screenHeight-3; myPixel_y++)
            {

                if (    (golfdiff->GetPixel(myPixel_x, myPixel_y) > 150 ||
                         golfdiff->GetPixel(myPixel_x, myPixel_y) < 100) && myPixel_y > max_flag  && myPixel_y > min_flag
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
    Point searchRegionPoint2a(screenWidth-1,screenHeight-1);

    //store the number of regions found
    int number_regions;

    objectDetection(satmask->GetElementPtr(0,0,0),
                    golfmap->GetElementPtr(0,0,0),
                    golfmap->GetNx(),
                    golfmap->GetNy(),
                    ballregion,
                    &number_regions,
                    indexes,
                    searchRegionPoint1a,
                    searchRegionPoint2a,
                    false);


    int tot_pix_in_regions=0;
    int zone_ticker = 0;

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
                ballregion[i].getNumberPixels()< 40 &&
                ballregion[i].getNumberPixels()> 2 &&
                //green->GetPixel(ball_centre_x,ball_centre_y) < 10 &&
                fieldmap->GetPixel(ball_centre_x,ball_centre_y,0) != 0
                )
        {
            /*
                printf("%3d     (%3d,%3d)   (%3d,%3d)   %6d            %3d->%3d\n", i,
                       ballregion[i].getMinX(), ballregion[i].getMinY(),
                       ballregion[i].getMaxX(), ballregion[i].getMaxY(),
                       ballregion[i].getNumberPixels(),
                       ballregion[i].getTopLineMinX(), ballregion[i].getTopLineMaxX() );*/
            tot_pix_in_regions += ballregion[i].getNumberPixels();
            zone_ticker +=1;
            possible_ball_x = ball_centre_x;
            possible_ball_y = ball_centre_y;
            x_difference = (possible_ball_x-ball_pixel_x)*(possible_ball_x-ball_pixel_x);
            y_difference = (possible_ball_y-ball_pixel_y)*(possible_ball_y-ball_pixel_y);

            int j = 0;
            bool newObject = true;

            while(isObject[j] == true){
                if(golfscreen_x[j] < ball_centre_x + 5 && golfscreen_x[j] > ball_centre_x - 5){
                    if(golfscreen_y[j] < ball_centre_y + 5 && golfscreen_y[j] > ball_centre_y - 5){
                        golfscreen_x[j] = ball_centre_x;
                        golfscreen_y[j] = ball_centre_y;
                        isObject[j] = true;
                        newObject = false;
                    }
                }
                j++;
            }

            if(newObject == true){
                golfscreen_x[j] = ball_centre_x;
                golfscreen_y[j] = ball_centre_y;
                isObject[j] = true;
            }


            if ((ball_pixel_x == 0 && ball_pixel_y == 0 && zone_ticker ==1) ||
                    (x_difference+y_difference < 1000))
            {
                ball_pixel_x = possible_ball_x;
                ball_pixel_y = possible_ball_y;
            }
        }
    }

    printf("Total pixels in all allocated regions = %d\n",tot_pix_in_regions);

    frameNumber[frame_count] = (int)frame_count + start;

    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
        {
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 0), myPixel_x, myPixel_y, 0);
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 1), myPixel_x, myPixel_y, 1);
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 2), myPixel_x, myPixel_y, 2);
        }
    }

    for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
    {

        output->SetPixel(255, flag_line, myPixel_y, 2);
        if(flag_line < screenWidth)      output->SetPixel(255, flag_line+1, myPixel_y, 2);
        if(flag_line > 0)                   output->SetPixel(255, flag_line-1, myPixel_y, 2);
        if(ball_pixel_x != 0)               output->SetPixel(255, ball_pixel_x, myPixel_y, 0);
    }

    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        output->SetPixel(255, myPixel_x, min_flag, 2);
        if(min_flag < screenWidth)      output->SetPixel(255, myPixel_x, min_flag+1, 2);
        if(min_flag > 0)                   output->SetPixel(255, myPixel_x, min_flag-1, 2);
        output->SetPixel(255, myPixel_x, max_flag ,1);
        if(max_flag < screenWidth)      output->SetPixel(255, myPixel_x, max_flag+1 ,1);
        if(max_flag > 0)                   output->SetPixel(255, myPixel_x, max_flag-1, 1);
        if(ball_pixel_y != 0)              output->SetPixel(255, myPixel_x, ball_pixel_y, 0);
    }



    std::cout << "FLAG LINE: " << flag_line << std::endl;
    std::cout << "FLAG MIN: " << min_flag << std::endl;
    std::cout << "FLAG MAX: " << max_flag << std::endl;

}

