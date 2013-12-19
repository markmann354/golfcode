bool segmenter(bbcvp::CPicture<unsigned char> picBuf,
               bbcvp::CPicture<unsigned char> green,
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
    int sat = 0;
    //int valueM = 0;

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
            sat = scaled_sat_finder(myInt_R, myInt_G, myInt_B);
            pixeldataarray[myPixel_x+myPixel_y*screenWidth]=greenhue;
            huehistogram[greenhue] += 1;
            golfsat->SetPixel(sat,myPixel_x,myPixel_y);

            if(myPixel_x % 10 == 0 && myPixel_y % 10 == 0){
            }

            if(sat<60){
                satmask->SetPixel(255,myPixel_x,myPixel_y);
            }
            else{
                satmask->SetPixel(0,myPixel_x,myPixel_y);
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


    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
        {
            if((pixeldataarray[myPixel_x+myPixel_y*screenWidth]<huemin - 30
                || pixeldataarray[myPixel_x+myPixel_y*screenWidth]> huemax + 30)
                    && green.GetPixel(myPixel_x, myPixel_y) == 255
                    //   || golfmap->GetPixel(myPixel_x, myPixel_y) == 255
                    //   || golfsat->GetPixel(myPixel_x, myPixel_y) < 60
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
    /*

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

    */

    /*

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

    */

    for (int myPixel_x = 0 ; myPixel_x < screenWidth; myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < screenHeight; myPixel_y++)
        {
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 0), myPixel_x, myPixel_y, 0);
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 1), myPixel_x, myPixel_y, 1);
            output->SetPixel(picBuf.GetPixel(myPixel_x, myPixel_y, 2), myPixel_x, myPixel_y, 2);
        }
    }

#define MAX_REGION 100000
    bbcvp::Region *ballregion = new bbcvp::Region[MAX_REGION];

    int indexes[MAX_REGION];

    Point searchRegionPoint1a(0,0);
    Point searchRegionPoint2a(screenWidth-1,screenHeight-1);

    //store the number of regions found
    int number_regions;

    objectDetection(golfmask->GetElementPtr(0,0,0),
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
    cout << "number of regions " << number_regions << endl;

    for (int ind=0; ind<number_regions; ind++)
    {
        int i = indexes[ind];
        int possible_ball_x = 0;
        int possible_ball_y = 0;
        int x_difference = 0;
        int y_difference = 0;
        int ball_centre_x = (ballregion[i].getMaxX()+ballregion[i].getMinX())/2;
        int ball_centre_y = (ballregion[i].getMaxY()+ballregion[i].getMinY())/2;
        if (//ballregion[i].getMinX()<ballregion[i].getMaxX()// &&
                ballregion[i].getNumberPixels()< 200 &&
                ballregion[i].getNumberPixels()> 20
                //  (ballregion[i].getMaxX()-ballregion[i].getMinX()) != 0 &&
                //  (ballregion[i].getMaxY()-ballregion[i].getMinY()) != 0 //&&
                // (ballregion[i].getMaxX()-ballregion[i].getMinX())/(ballregion[i].getMaxY()-ballregion[i].getMinY())<3 &&
                // (ballregion[i].getMaxY()-ballregion[i].getMinY())/(ballregion[i].getMaxX()-ballregion[i].getMinX())<3
                )
        {
            for(int k = ball_centre_x-2; k < ball_centre_x +3 ; k++){
                for(int j = ball_centre_y-2; j < ball_centre_y +3 ; j++){
                    output->SetPixel(255, k, j, 0);
                    output->SetPixel(0, k, j, 1);
                    output->SetPixel(0, k, j, 2);
                }
            }
            //std::cout << "Object(" << i << ") : " << ball_centre_x << "," << ball_centre_y << std::endl;
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

    cout << "zone_ticker " << zone_ticker << endl;

    /*
    printf("Total pixels in all allocated regions = %d\n",tot_pix_in_regions);

    frameNumber[frame_count] = (int)frame_count + start;



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
*/

    delete huehistogram;
    delete pixeldataarray;
    delete pixeldataarrayRGB;
    delete temparray2;
}



void drawnumber(bbcvp::CPicture<unsigned char> *output, int x, int y, int colour, int number){
    if (number == 0){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x-1,y+1,0);
        output->SetPixel(colour,x-1,y+1,1);
        output->SetPixel(colour,x-1,y+1,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
    }

    if (number == 1){
        output->SetPixel(colour,x,y+1,0);
        output->SetPixel(colour,x,y+1,1);
        output->SetPixel(colour,x,y+1,2);
        output->SetPixel(colour,x,y-1,0);
        output->SetPixel(colour,x,y-1,1);
        output->SetPixel(colour,x,y-1,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
    }

    if (number == 2){
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
        output->SetPixel(colour,x-1,y+1,0);
        output->SetPixel(colour,x-1,y+1,1);
        output->SetPixel(colour,x-1,y+1,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
    }
    if (number == 3){
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
    }
    if (number == 4){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
    }

    if (number == 5){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
    }

    if (number == 6){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x-1,y+1,0);
        output->SetPixel(colour,x-1,y+1,1);
        output->SetPixel(colour,x-1,y+1,2);
    }

    if (number == 7){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
    }

    if (number == 8){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x-1,y+1,0);
        output->SetPixel(colour,x-1,y+1,1);
        output->SetPixel(colour,x-1,y+1,2);
        output->SetPixel(colour,x-1,y+2,0);
        output->SetPixel(colour,x-1,y+2,1);
        output->SetPixel(colour,x-1,y+2,2);
        output->SetPixel(colour,x,y+2,0);
        output->SetPixel(colour,x,y+2,1);
        output->SetPixel(colour,x,y+2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
    }

    if (number == 9){
        output->SetPixel(colour,x+1,y,0);
        output->SetPixel(colour,x+1,y,1);
        output->SetPixel(colour,x+1,y,2);
        output->SetPixel(colour,x+1,y-1,0);
        output->SetPixel(colour,x+1,y-1,1);
        output->SetPixel(colour,x+1,y-1,2);
        output->SetPixel(colour,x+1,y-2,0);
        output->SetPixel(colour,x+1,y-2,1);
        output->SetPixel(colour,x+1,y-2,2);
        output->SetPixel(colour,x+1,y+1,0);
        output->SetPixel(colour,x+1,y+1,1);
        output->SetPixel(colour,x+1,y+1,2);
        output->SetPixel(colour,x+1,y+2,0);
        output->SetPixel(colour,x+1,y+2,1);
        output->SetPixel(colour,x+1,y+2,2);
        output->SetPixel(colour,x-1,y,0);
        output->SetPixel(colour,x-1,y,1);
        output->SetPixel(colour,x-1,y,2);
        output->SetPixel(colour,x-1,y-1,0);
        output->SetPixel(colour,x-1,y-1,1);
        output->SetPixel(colour,x-1,y-1,2);
        output->SetPixel(colour,x-1,y-2,0);
        output->SetPixel(colour,x-1,y-2,1);
        output->SetPixel(colour,x-1,y-2,2);
        output->SetPixel(colour,x,y-2,0);
        output->SetPixel(colour,x,y-2,1);
        output->SetPixel(colour,x,y-2,2);
        output->SetPixel(colour,x,y,0);
        output->SetPixel(colour,x,y,1);
        output->SetPixel(colour,x,y,2);
    }
}

bool motion_segmenter(bbcvp::CPicture<unsigned char> picBuf,
                      bbcvp::CPicture<unsigned char> picBefBuf,
                      bbcvp::CPicture<unsigned char> green,
                      //                      double golfscreen_x[],
                      //                      double golfscreen_y[],
                      //                      bool isObject[],
                      bbcvp::CPicture<unsigned char> *prev2mask
                      //                      bbcvp::CPicture<unsigned char> *prevmap
                      ){

    //this bit gives you the first mask for the motion detection

    int screenWidth = 0;
    int screenHeight = 0;
    int ball_pixel_x = 0;
    int ball_pixel_y = 0;

    //  cout << "bugger" << endl;

    for(int xm = 1;xm<picBuf.GetNx()-1;xm++)
    {
        for(int ym = 1;ym<picBuf.GetNy()-1;ym++)
        {

            if (    ((picBuf.GetPixel(xm, ym, 1)   > picBefBuf.GetPixel(xm, ym, 1)+30)   || (picBuf.GetPixel(xm, ym, 1)   < picBefBuf.GetPixel(xm, ym, 1)-30)   ||
                    (picBuf.GetPixel(xm+1, ym, 1) > picBefBuf.GetPixel(xm+1, ym, 1)+30) || (picBuf.GetPixel(xm+1, ym, 1) < picBefBuf.GetPixel(xm+1, ym, 1)-30) ||
                    (picBuf.GetPixel(xm-1, ym, 1) > picBefBuf.GetPixel(xm-1, ym, 1)+30) || (picBuf.GetPixel(xm-1, ym, 1) < picBefBuf.GetPixel(xm-1, ym, 1)-30) ||
                    (picBuf.GetPixel(xm, ym+1, 1) > picBefBuf.GetPixel(xm, ym+1, 1)+30) || (picBuf.GetPixel(xm, ym+1, 1) < picBefBuf.GetPixel(xm, ym+1, 1)-30) ||
                    (picBuf.GetPixel(xm, ym-1, 1) > picBefBuf.GetPixel(xm, ym-1, 1)+30) || (picBuf.GetPixel(xm, ym-1, 1) < picBefBuf.GetPixel(xm, ym-1, 1)-30)) &&
                    green.GetPixel(xm,ym) == 255
                    )
            {
                prev2mask->SetPixel(255, xm, ym);
            }
            else
                prev2mask->SetPixel(0, xm, ym);
        }
    }

    /*

screenWidth = picBuf.GetNx();
screenHeight = picBuf.GetNy();

//cout << "screenwidth " << screenWidth << endl;
//cout << "screenheight " << screenHeight << endl;

//bbcvp::Erode(*prev2mask, *prev2mask, 4);


#define MAX_REGION 100000
    bbcvp::Region *ballregion = new bbcvp::Region[MAX_REGION];

    int indexes[MAX_REGION];

    Point searchRegionPoint1a(0,0);
    Point searchRegionPoint2a(screenWidth-1,screenHeight-1);

    //store the number of regions found
    int number_regions;

    objectDetection(prev2mask->GetElementPtr(0,0,0),
                    prevmap->GetElementPtr(0,0,0),
                    prevmap->GetNx(),
                    prevmap->GetNy(),
                    ballregion,
                    &number_regions,
                    indexes,
                    searchRegionPoint1a,
                    searchRegionPoint2a,
                    false);


    int tot_pix_in_regions=0;
    int zone_ticker = 0;
    cout << "number of regions mover " << number_regions << endl;

    for (int ind=0; ind<number_regions; ind++)
    {
        int i = indexes[ind];
        int possible_ball_x = 0;
        int possible_ball_y = 0;
        int x_difference = 0;
        int y_difference = 0;
        int ball_centre_x = (ballregion[i].getMaxX()+ballregion[i].getMinX())/2;
        int ball_centre_y = (ballregion[i].getMaxY()+ballregion[i].getMinY())/2;
        if (//ballregion[i].getMinX()<ballregion[i].getMaxX()// &&
                ballregion[i].getNumberPixels()< 600 &&
                ballregion[i].getNumberPixels()> 40
              //  (ballregion[i].getMaxX()-ballregion[i].getMinX()) != 0 &&
              //  (ballregion[i].getMaxY()-ballregion[i].getMinY()) != 0 //&&
               // (ballregion[i].getMaxX()-ballregion[i].getMinX())/(ballregion[i].getMaxY()-ballregion[i].getMinY())<3 &&
               // (ballregion[i].getMaxY()-ballregion[i].getMinY())/(ballregion[i].getMaxX()-ballregion[i].getMinX())<3
                )
        {
      /*      for(int k = ball_centre_x-2; k < ball_centre_x +3 ; k++){
                for(int j = ball_centre_y-2; j < ball_centre_y +3 ; j++){
                    prev2mask->SetPixel(128, k, j);
                }
           } */
    //std::cout << "Object(" << i << ") : " << ball_centre_x << "," << ball_centre_y << std::endl;
    /*
                printf("%3d     (%3d,%3d)   (%3d,%3d)   %6d            %3d->%3d\n", i,
                       ballregion[i].getMinX(), ballregion[i].getMinY(),
                       ballregion[i].getMaxX(), ballregion[i].getMaxY(),
                       ballregion[i].getNumberPixels(),
                       ballregion[i].getTopLineMinX(), ballregion[i].getTopLineMaxX() );*/
    /*          tot_pix_in_regions += ballregion[i].getNumberPixels();
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

    cout << "zone_ticker " << zone_ticker << endl;



*/
    //cout << "bugger 2" << endl;



}





bool uber_segmenter(  bbcvp::CPicture<unsigned char> prev2mask,
                      double golfscreen_x[],
                      double golfscreen_y[],
                      bool isObject[],
                      bbcvp::CPicture<unsigned char> *prevmap
                      ){

    int screenWidth  = prev2mask.GetNx();
    int screenHeight = prev2mask.GetNy();
    int ball_pixel_x = 0;
    int ball_pixel_y = 0;

    double ticker = 0;
    double average_x = 0;
    double average_y = 0;
    cout << "bugger" << endl;
    for(int i=0; i<screenWidth; i++){
        for(int j=0; j<screenHeight; j++){
            if(prev2mask.GetPixel(i,j)==255){
                //     cout << "hi" << endl;
                ticker += 1;
                average_x = i/ticker + (ticker-1)*average_x/ticker;
                average_y = i/ticker + (ticker-1)*average_y/ticker;
            }
        }
    }


#define MAX_REGION 100000
    bbcvp::Region *ballregion = new bbcvp::Region[MAX_REGION];

    int indexes[MAX_REGION];

    Point searchRegionPoint1a(0,0);
    Point searchRegionPoint2a(screenWidth-1,screenHeight-1);

    //store the number of regions found
    int number_regions;

    objectDetection(prev2mask.GetElementPtr(0,0,0),
                    prevmap->GetElementPtr(0,0,0),
                    prevmap->GetNx(),
                    prevmap->GetNy(),
                    ballregion,
                    &number_regions,
                    indexes,
                    searchRegionPoint1a,
                    searchRegionPoint2a,
                    false);


    int tot_pix_in_regions=0;
    int zone_ticker = 0;
    cout << "number of regions mover " << number_regions << endl;

    for (int ind=0; ind<number_regions; ind++)
    {
        int i = indexes[ind];
        int possible_ball_x = 0;
        int possible_ball_y = 0;
        int x_difference = 0;
        int y_difference = 0;
        int ball_centre_x = (ballregion[i].getMaxX()+ballregion[i].getMinX())/2;
        int ball_centre_y = (ballregion[i].getMaxY()+ballregion[i].getMinY())/2;
        if (//ballregion[i].getMinX()<ballregion[i].getMaxX()// &&
                ballregion[i].getNumberPixels()< 600 &&
                ballregion[i].getNumberPixels()> 20
                //  (ballregion[i].getMaxX()-ballregion[i].getMinX()) != 0 &&
                //  (ballregion[i].getMaxY()-ballregion[i].getMinY()) != 0 //&&
                // (ballregion[i].getMaxX()-ballregion[i].getMinX())/(ballregion[i].getMaxY()-ballregion[i].getMinY())<3 &&
                // (ballregion[i].getMaxY()-ballregion[i].getMinY())/(ballregion[i].getMaxX()-ballregion[i].getMinX())<3
                )
        {
            /*      for(int k = ball_centre_x-2; k < ball_centre_x +3 ; k++){
                for(int j = ball_centre_y-2; j < ball_centre_y +3 ; j++){
                    prev2mask->SetPixel(128, k, j);
                }
           } */
            //std::cout << "Object(" << i << ") : " << ball_centre_x << "," << ball_centre_y << std::endl;

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

            int j = 0;
            bool newObject = true;

            while(isObject[j] == true){
                if(golfscreen_x[j] < ball_centre_x + 50 && golfscreen_x[j] > ball_centre_x - 50){
                    if(golfscreen_y[j] < ball_centre_y + 50 && golfscreen_y[j] > ball_centre_y - 50){
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


    cout << "zone_ticker " << zone_ticker << endl;
}

bool object_sorter(double M1golfscreen_x[],
                   double M1golfscreen_y[],
                   double M2golfscreen_x[],
                   double M2golfscreen_y[],
                   int blacklist1_x[],
                   int blacklist1_y[],
                   int blacklist2_x[],
                   int blacklist2_y[],
                   int object_Counter,
                   int object_Counter2,
                   int bl1,
                   int bl2,
                   long double CamPosX,
                   long double CamPosY,
                   long double CamPosZ,
                   long double TwoCamPosX,
                   long double TwoCamPosY,
                   long double TwoCamPosZ,
                   bbcvp::CPicture<unsigned char> regGrid1,
                   bbcvp::CPicture<unsigned char> regGrid2,
                   double minimizedValue[],
                   int camOneObject[],
                   int camTwoObject[],
                   int nextsmallest_minimizedValue[],
                   int nextsmallest_camOneObject[],
                   int nextsmallest_camTwoObject[],
                   bool object1assigned[],
                   bool object2assigned[],
                   int zzz,
                   bbcvp::PoseEstPanTiltHead *myPoseEst,
                   bbcvp::PoseEstPanTiltHead *TwomyPoseEst,
                   double objectX[],
                   double objectY[],
                   double objectZ[],
                   int smallestCounter,
                   int offlimit
        ){

    double px = 0;
    double py = 0;
    double pz = 0;

    double px7 = 0;
    double py7 = 0;
    double pz7 = 0;

    double ballWorldX = 0;
    double ballWorldY = 0;
    double ballWorldZ = 0;

    double ball2WorldX = 0;
    double ball2WorldY = 0;
    double ball2WorldZ = 0;

    int z_counter = 0;
    bool found_match = false;


    int cr = 0;


    for (int l = 0; l < object_Counter; l++){
        int cont = true;

        for(int p = 0; p < bl1; p++){
            if(        M1golfscreen_x[l]<blacklist1_x[p]+offlimit
                       && M1golfscreen_x[l]>blacklist1_x[p]-offlimit
                       && M1golfscreen_y[l]<blacklist1_y[p]+offlimit
                       && M1golfscreen_y[l]>blacklist1_y[p]-offlimit
                       && M1golfscreen_x[l] != 0
                       && M1golfscreen_y[l] != 0
                       ){
                blacklist1_x[p]=M1golfscreen_x[l];
                blacklist1_y[p]=M1golfscreen_y[l];
                cont = false;}
        }
        if(cont == true){
            myPoseEst->lineOfSight(M1golfscreen_x[l],M1golfscreen_y[l], &ballWorldX, &ballWorldY, &ballWorldZ);

            double minimizer = 10000000000000;

            int Tassigned = object_Counter2+1;
            for (int t = 0; t < object_Counter2; t++){

                int cont2 = true;
                for(int p = 0; p < bl2; p++){
                    if(        M2golfscreen_x[t]<blacklist2_x[p]+offlimit
                               && M2golfscreen_x[t]>blacklist2_x[p]-offlimit
                               && M2golfscreen_y[t]<blacklist2_y[p]+offlimit
                               && M2golfscreen_y[t]>blacklist2_y[p]-offlimit
                               && M2golfscreen_x[t] != 0
                               && M2golfscreen_y[t] != 0
                               ){
                        blacklist2_x[p]=M2golfscreen_x[t];
                        blacklist2_y[p]=M2golfscreen_y[t];
                        cont2 = false;}
                }
                if(cont2 == true){
                    TwomyPoseEst->lineOfSight(M2golfscreen_x[t],M2golfscreen_y[t], &ball2WorldX, &ball2WorldY, &ball2WorldZ);
                    double u = ballWorldX*ball2WorldX + ballWorldY*ball2WorldY + ballWorldZ*ball2WorldZ;

                    double w0x = CamPosX-TwoCamPosX;
                    double w0y = CamPosY-TwoCamPosY;
                    double w0z = CamPosZ-TwoCamPosZ;

                    double bb = ballWorldX*ball2WorldX + ballWorldY*ball2WorldY + ballWorldZ*ball2WorldZ;
                    double aa = ballWorldX*ballWorldX + ballWorldY*ballWorldY + ballWorldZ*ballWorldZ;
                    double cc = ball2WorldX*ball2WorldX + ball2WorldY*ball2WorldY + ball2WorldZ*ball2WorldZ;
                    double dd = ballWorldX*w0x + ballWorldY*w0y + ballWorldZ*w0z;
                    double ee = ball2WorldX*w0x + ball2WorldY*w0y + ball2WorldZ*w0z;

                    double Sc = (bb*ee-cc*dd)/(aa*cc-bb*bb);
                    double Tc = (aa*ee-bb*dd)/(aa*cc-bb*bb);

                    if (u != 1){

                        long double p1x = CamPosX + Sc*ballWorldX;
                        long double p1y = CamPosY + Sc*ballWorldY;
                        long double p1z = CamPosZ + Sc*ballWorldZ;

                        long double p2x = TwoCamPosX + Tc*ball2WorldX;
                        long double p2y = TwoCamPosY + Tc*ball2WorldY;
                        long double p2z = TwoCamPosZ + Tc*ball2WorldZ;

                        bool alreadyFoundOnOne = false;
                        bool alreadyFoundOnTwo = false;
                        int foundOne = smallestCounter+10;
                        int foundTwo = smallestCounter+10;

                        double minimized = sqrt((p1x-p2x)*(p1x-p2x)+(p1y-p2y)*(p1y-p2y)+(p1z-p2z)*(p1z-p2z)); //(p1x-p2x)*(p1x-p2x)+(p1y-p2y)*(p1y-p2y)+(p1z-p2z)*(p1z-p2z)
                        int one = regGrid1.GetPixel(M1golfscreen_x[l],M1golfscreen_y[l]);
                        int two = regGrid2.GetPixel(M2golfscreen_x[t],M2golfscreen_y[t]);

                        if(minimized < minimizer
                                && (zzz!=0 || ((one == two || one == two+1 || one == two-1 || one == two+16 || one == two-16 || one == two-15 || one == two-17 || one == two+17 || one == two+15)
                                               && one != 0 && two !=0))
                                ){
                            found_match = true;

                            for(int b = 0; b < smallestCounter; b++){
                                if((camOneObject[b] == l || camTwoObject[b] == t)){
                                    if (camOneObject[b] == l){
                                        alreadyFoundOnOne = true;
                                        foundOne = b;
                                    }
                                    if (camTwoObject[b] == t){
                                        alreadyFoundOnTwo = true;
                                        foundTwo = b;
                                    }
                                }
                            }
                            if(alreadyFoundOnTwo == true){

                                if (minimized < minimizedValue[foundTwo]){

                                    if(l != camOneObject[z_counter+1] && (nextsmallest_camOneObject[z_counter+1] != camOneObject[z_counter+1] || camOneObject[z_counter+1] == smallestCounter+10)){
                                        nextsmallest_camOneObject[z_counter+1] = camOneObject[z_counter+1];
                                    }
                                    camOneObject[z_counter+1] = l;

                                    if(t != camTwoObject[z_counter+1] && (nextsmallest_camTwoObject[z_counter+1] != camTwoObject[z_counter+1] || camTwoObject[z_counter+1] == smallestCounter+10)){
                                        nextsmallest_camTwoObject[z_counter+1] = camTwoObject[z_counter+1];
                                    }
                                    camTwoObject[z_counter+1] = t;

                                    minimizer = minimized;

                                    if(minimizer != minimizedValue[z_counter+1] && (nextsmallest_minimizedValue[z_counter+1] != minimizedValue[z_counter+1] || minimizedValue[z_counter+1] == smallestCounter+10)){
                                        nextsmallest_minimizedValue[z_counter+1] = minimizedValue[z_counter+1];
                                    }
                                    minimizedValue[z_counter+1] = minimizer;

                                    minimizedValue[foundTwo] = nextsmallest_minimizedValue[foundTwo];
                                    camOneObject[foundTwo] = nextsmallest_camOneObject[foundTwo];
                                    camTwoObject[foundTwo] = nextsmallest_camTwoObject[foundTwo];

                                    nextsmallest_minimizedValue[foundTwo] = 0;
                                    nextsmallest_camOneObject[foundTwo] = 0;
                                    nextsmallest_camTwoObject[foundTwo] = 0;

                                    px = p1x;
                                    py = p1y;
                                    pz = p1z;

                                    px7 = p2x;
                                    py7 = p2y;
                                    pz7 = p2z;

                                    object1assigned[l]=true;
                                    object1assigned[nextsmallest_camOneObject[foundTwo]]=false;
                                    Tassigned=t;

                                }
                            }

                            else {

                                nextsmallest_camOneObject[z_counter+1] = camOneObject[z_counter+1];
                                nextsmallest_camTwoObject[z_counter+1] = camTwoObject[z_counter+1];
                                camOneObject[z_counter+1] = l;
                                camTwoObject[z_counter+1] = t;

                                minimizer = minimized;

                                minimizedValue[z_counter+1] = minimizer;

                                px = p1x;
                                py = p1y;
                                pz = p1z;

                                px7 = p2x;
                                py7 = p2y;
                                pz7 = p2z;

                                object1assigned[l]=true;

                                Tassigned=t;

                            }

                        }

                    }

                }

            }

            object2assigned[Tassigned]=true;

            cr+=1;

            if(found_match == true){
                z_counter+=1;
                objectX[z_counter] = (px+px7)/2;
                objectY[z_counter] = (py+py7)/2;
                objectZ[z_counter] = (pz+pz7)/2;
            }
            found_match = false;

        }
    }


}










