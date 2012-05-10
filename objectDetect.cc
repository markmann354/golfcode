
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct Point
{
int x;
int y;

Point()
{
   x = 0;
   y = 0;
}
Point(int a, int b)
{
   x = a;
   y = b;
}

};

/*
 * Function by Jonathan McKinnell to produce RGB object numbering based on size
 * Modified by Robert Dawes then Martin Nicholson
 */
void objectDetection(unsigned char* img, unsigned char* objident, int width, int height, bbcvp::Region *region, int *num_regions, int *indexes,
													Point roiPoint1, Point roiPoint2, bool efficiencyChecks)
{
	bool debug(true);
	// objident is the ID that is always the same across an object as it is discovered and is only incremented
	// when a 'new' object is found. The ID is never reset. During one of the later loops, if objects get joined
	// together they take the larger ID, as they did before.

	// Error checking on the provided region of interest
	bool roiArgumentError(false);
	if (roiPoint1.x >= roiPoint2.x) // Check that the x coordinates are the correct way around
        {	roiArgumentError = true;
            std::cerr << "ok here 1"<< std::endl;}
	if (roiPoint1.y >= roiPoint2.y) // Check that the y coordinates are the correct way around
        {	roiArgumentError = true;
            std::cerr << "ok here 2"<< std::endl;}
	if (roiPoint1.x < 0 || roiPoint1.y < 0) // Check that the ROI starts beyond (0,0)
        {	roiArgumentError = true;
            std::cerr << "ok here 3"<< std::endl;}
	if (roiPoint2.x >= width || roiPoint2.y >= height) // Check that the ROI ends within (width,height)
        {	roiArgumentError = true;
            std::cerr << "ok here 4"<< std::endl;}

	if (roiArgumentError)
	{
		std::cerr << "diverSearch: Invalid region of interest provided for object detection" << std::endl;
		return;
	}


	// Create new integer array to take each pixel value and initialise to zero
	//int * objident = new int [height*width];

	for (int j = 0; j < (height*width); j++)
	{
			objident[j] = 0;
	}

	int objidentcount = 1;
	int jloop, iloop;

	// Phase 1: Loop forwards through the image and set the w_min and the h_min
	if(debug)
        //	std::cout << "diverSearch: Phase 1: Begin. Image width = "<< width << " and height = " << height << std::endl;
	for (int j = roiPoint1.y; j <= roiPoint2.y; j++)
	{
		for (int i = roiPoint1.x; i <= roiPoint2.x; i++)
		{
			unsigned char row = img[(i+((j)*width))];
			if (row == 255) // Take only the white pixels
			{
				int connection = 0;
				//if(  i > 3 && i < width - 4 && j > 3 && j < height - 4 ) // UPDATED BOUNDARY CONDITIONS
				if ( i > roiPoint1.x && i < roiPoint2.x && j > roiPoint1.y && j < roiPoint2.y )
				{
					// If not in border 2 rows/columns
					if ( i > roiPoint1.x+1 && i < roiPoint2.x-2 && j > roiPoint1.y+1 && j < roiPoint2.y-2 )
						for(jloop = -2; ((jloop <= 2)&&(connection==0)); jloop++)
							for(iloop = -2; ((iloop <= 2)&&(connection==0)); iloop++)
							{
								row = img[(i+iloop+((j+jloop)*width))];
								if(row == 255)
									connection = 1;
							}
					// If region connected to another
					if(connection == 1)
					{
						// If not in border 2 rows/columns
						if ( i > roiPoint1.x+1 && i < roiPoint2.x-2 && j > roiPoint1.y+1 && j < roiPoint2.y-2 )
							for(jloop = -2; jloop <= 2; jloop++)
								for(iloop = -2; iloop <= 2; iloop++)
								{
									if(i+iloop+(j+jloop)*width > width*height - 1)
										std::cerr << "diverSearch: Phase 1: LOOP ERROR. i+iloop+(j+jloop)*width out of bounds i = " <<
											i << " j = " << j << " iloop = " << iloop << " jloop = " << jloop << std::endl;
									objident[i+j*width] = MAX(objident[i+iloop+(j+jloop)*width],objident[i+j*width]);
								}
						else // If in the border
						{
							// Find the largest object label on neighbour
							objident[i+j*width] = MAX(objident[i-1+(j-1)*width],MAX(objident[i+(j-1)*width],MAX(objident[i+1+(j-1)*width],MAX(objident[i-1+(j)*width],0))));
						}
						if (objident[i+j*width] == 0) // Give a new label
						{
							objident[i+j*width] = objidentcount;
							objidentcount++;
						}
					}
				}
			}
		}
	}
	if (efficiencyChecks)
	{
		// First efficiency check: bail if a large number of objects are found
		int maxObjident(0);
		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++)
				if (objident[i+j*width] > maxObjident)
					maxObjident = objident[i+j*width];
		if (maxObjident > 50)
		{
			if (debug)
                        //	std::cout << "diverSearch: Large number of objects found (" << maxObjident <<
                        //		") which probably means it is not the entry point. Leaving object detection early." << std::endl;
			return;
		}
	}
	// Phase 2. Loop backwards through the image. Set the w_max and the h_max
	if (debug)
        //	std::cout << "diverSearch: Begin Phase 2" << std::endl;
	for (int j = roiPoint2.y; j >= roiPoint1.y; j--)
	{
		for (int i = roiPoint2.x; i >= roiPoint1.x; i--)
		{
			unsigned char row = img[i+(j*width)];
			if( row == 255 ) // Take only the white pixels
			{
				// If in the region of interest
				if ( i > roiPoint1.x && i < roiPoint2.x && j > roiPoint1.y && j < roiPoint2.y )
				{
					// Exclude 2 edge rows/columns
					if ( i > roiPoint1.x+1 && i < roiPoint2.x-2 && j > roiPoint1.y+1 && j < roiPoint2.y-2 )
						for(jloop = -2; jloop <= 2; jloop++)
							for(iloop = -2; iloop <= 2; iloop++)
							{
								objident[i+j*width] = MAX(objident[i+iloop+(j+jloop)*width],objident[i+j*width]);
								if(i+iloop+(j+jloop)*width > width*height)
									std::cerr << "diverSearch: Phase 2: LOOP ERROR. i+iloop+(j+jloop)*width out of bounds i = " <<
										i << " j = " << j << " iloop = " << iloop << " jloop = " << jloop << std::endl;
							}
					// The 2 edge rows/columns
					else
						objident[i+j*width] = MAX(objident[i-1+(j-1)*width],MAX(objident[i+(j-1)*width],MAX(objident[i+1+(j-1)*width],
							MAX(objident[i-1+(j)*width],MAX(objident[i+1+(j)*width],
							MAX(objident[i-1+(j+1)*width],MAX(objident[i+(j+1)*width],objident[i+1+(j+1)*width])))))));
				}
			}
		}
	}
	if (debug)
        //	std::cout << "diverSearch: Begin Final Phase" << std::endl;

	// Final Phase. Forward pass
	for (int j = roiPoint1.y; j <= roiPoint2.y; j++)
	{


		for (int i = roiPoint1.x; i <= roiPoint2.x; i++)
		{
			unsigned char row = img[(i+((j)*width))];
			if( row == 255) // If white pixels
			{
				// If in the region of interest
				if ( i > roiPoint1.x && i < roiPoint2.x && j > roiPoint1.y && j < roiPoint2.y )
				{
					// Not edge 2 rows/columns
					if( i > roiPoint1.x+1 && i < roiPoint2.x-2 && j > roiPoint1.y+1 && j < roiPoint2.y-2)
						for(jloop = -2; jloop <= 2; jloop++)
							for(iloop = -2; iloop <= 2; iloop++)
							{
								objident[i+j*width] = MAX(objident[i+iloop+(j+jloop)*width],objident[i+j*width]);
								if(i+iloop+(j+jloop)*width > width*height)
									std::cerr << "diverSearch: Phase 3: LOOP ERROR. i+iloop+(j+jloop)*width out of bounds i = " <<
										i << " j = " << j << " iloop = " << iloop << " jloop = " << jloop << std::endl;
							}
					// Edge 2 rows/columns
					else
						objident[i+j*width] = MAX(objident[i-1+(j-1)*width],MAX(objident[i+(j-1)*width],
								MAX(objident[i+1+(j-1)*width],MAX(objident[i-1+(j)*width],MAX(objident[i+1+(j)*width],
								MAX(objident[i-1+(j+1)*width],MAX(objident[i+(j+1)*width],objident[i+1+(j+1)*width])))))));
				}
			}
		}
	}

	// Build new arrays for storing the object variables
	int * scores = new int [objidentcount];
	int * w_min = new int [objidentcount];
	int * h_min = new int [objidentcount];
	int * w_max = new int [objidentcount];
	int * h_max = new int [objidentcount];


    const bbcvp::Region *r = new bbcvp::Region();

	// Initialise the arrays
	for (int i=0; i < objidentcount; i++)
	{
        indexes[i] = i;
		scores[i] = 0;
		w_max[i] = 0;
		h_max[i] = 0;
		w_min[i] = width;
		h_min[i] = height;

        region[i] = *r;
	}


	// Find the size and dimensions of all the different objects
	int nonzero(0), biggestObject(0), biggestScore(0);
	// Loop through the region of interest
	for (int j = roiPoint1.y; j <= roiPoint2.y; j++)
	{
		for (int i = roiPoint1.x; i <= roiPoint2.x; i++)
		{
			int index = objident[i+j*width];
			if (index > 0) // Object located at that pixel
			{
				region[index].addPixel(i,j);

				if (scores[index] == 0) // Store number of scores that are greater than zero
					nonzero++;
				scores[index]++; // Increment the score
				if(i < w_min[index]) // Update the minimum width value
					w_min[index] = i;
				if(j < h_min[index]) // Update the minimum height value
					h_min[index] = j;
				if(i > w_max[index]) // Update the maximum width value
					w_max[index] = i;
				if(j > h_max[index]) // Update the maximum height value
					h_max[index] = j;

                if (scores[index] > biggestScore)
                {
					biggestScore = scores[index];
				}

			}
		}
    }


   //debug = true;
	// Second efficiency check: bail if no meaningful objects are found.
	if (efficiencyChecks)
	{
		if ((nonzero == 0 )|| (biggestScore < 10))
		{
			if (debug)
                //		std::cout << "diverSearch: No meaningful objects found. Leaving object detection early." << std::endl;
			return;
		}
	}



   int tempS;
   int tempI;
   for (int p=0; p<objidentcount; p++)
   {
      for (int q=p; q <objidentcount; q++)
	  {
		if ( scores[p] < scores[q])
		{
		    tempS = scores[p];
		    tempI = indexes[p];

		    scores[p] = scores[q];
		    indexes[p] = indexes[q];

		    scores[q] = tempS;
		    indexes[q] = tempI;
		}
	  }
   }


  //std::cout << "nonzero" << nonzero <<  std::endl;

   *num_regions = nonzero;


	// Clean up memory
	delete [] scores;
	delete [] w_min;
	delete [] h_min;
	delete [] w_max;
	delete [] h_max;

}
