/* Student Name: Harun Zengin
* Student Number: 2012400081
* Compile Status: Compiling
* Program Status: Working
* Notes: The program works as wanted in the project description. 
*        But the arrays are not dynamically allocated because for a 2D array that doesn't work. 
*        There are code sections which are commented out. I didn't delete them to show you my try and testing behaviour.
*        The int calculated variable is only created for testing issues.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"

int main(int argc, char* argv[])
{
    if(argc != 3){
        printf("Arguments not supplied!");  
        return 0;
    }   
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank, size
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int pixel = 200;    // can be changed for different images
    int threshold = 10;  //the threshold value
    int row_per_process = pixel/(size-1);
    int mainImage[pixel + row_per_process][pixel];
    int calculatedImage[pixel + row_per_process][pixel-4];
    int i,j;                                                //for all 2D array iterations
    /* Dynamically allocation doesn2t work
    for (i=0; i<pixel + (row_per_process; i++)
         mainImage[i] = (int *)malloc(pixel * sizeof(int));*/

    if (rank == 0) {
        FILE *myFile;
        myFile = fopen(argv[1], "r");

        //read file into array mainImage
        
        for (i = 0; i < row_per_process; i++){
            for (j = 0; j < pixel; j++){
                mainImage[i][j] = 0;
            }
        }

        for (i = row_per_process; i < pixel + row_per_process; i++){
            for(j = 0; j<pixel; j++){
                fscanf(myFile, "%d", &mainImage[i][j]);
            }
        }
        /*
        //Only to print if current file is read rigth or not!
        FILE *fa = fopen("input read","w");

           for (i = 0; i < pixel + row_per_process; i++){
                for(j = 0; j<pixel; j++){
                    fprintf(fa, "%d ", mainImage[i][j]);
                }
                fprintf(fa, "\n" );
            }
        fclose(fa);
        */

    }
    int subImage [row_per_process][pixel];
       /*Commented out because dynamic array doesnt work for 2D Array
        for (i=0; i< row_per_process; i++)
            subImage[i] = (int *)malloc(pixel * sizeof(int));*/


    MPI_Scatter(mainImage,row_per_process*pixel, MPI_INT,     //send (200/size-1) * 200 elements to each process
                subImage, row_per_process*pixel, MPI_INT,      //receive same size in subImage
                0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
/*  Commented out because dynamic array doesnt work for 2D Array
    if(rank != 0){
        for (i=0; i<200 + (200/(size-1)); i++){
            free(mainImage[i]);
        }
    }*/
    int calculated = 0; //for testing
/*
    To send and receive the upper and lower parts of the array, these arrays are defined in each process and 
    are maintaining the synchronization

*/

    int send_upper[pixel] ;                 //upper part of the current subImage to be sent
    int send_lower[pixel] ;                 //lower part of the current subImage to be sent
    for (i=0;i<pixel;i++){
        send_upper[i] = subImage[0][i];
        int lower = row_per_process-1;
        send_lower[i] = subImage[lower][i];
    }
    int receive_upper[pixel];               //upper part of lower rank to be received
    int receive_lower[pixel];               //lower part of upper rank to be received
    int smooIma [row_per_process][pixel-2];    //the smoothed subImage

    if (rank !=0 && rank != size -1){
        MPI_Send(
            &send_lower,        //data pointer
            pixel,                  //count
            MPI_INT,            //type
            rank+1,                  //receiver
            0,                  //tag
            MPI_COMM_WORLD);
    }
    if (rank >1){
        MPI_Recv(&receive_upper,pixel,MPI_INT,rank-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    if (rank >1){
        MPI_Send(&send_upper,pixel,MPI_INT,rank-1,1,MPI_COMM_WORLD);
    }
    if (rank !=0 && rank != size -1){
        MPI_Recv(&receive_lower,pixel,MPI_INT,rank+1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    //enter smoothing part
    if(rank != 0 ){
        for (i=0;i<row_per_process;i++){
            if(!((rank == 1 && i == 0)||(rank == size -1 && i == row_per_process-1))) {
                for (j=1; j<pixel-1; j++){
                    calculated = 16;
                    if(i==0 && pixel ==  size-1){
                        smooIma[i][j-1] = ( receive_upper[j-1] + receive_upper[j] + receive_upper[j+1] + 
                                            subImage[i][j-1] + subImage[i][j] + subImage[i][j+1] +                  //Smoothes image if every process has one row
                                            receive_lower[j-1]  + receive_lower[j]  + receive_lower[j+1])/9;
                    } else if(i == 0 && !(rank ==1 && size -1== pixel)){
                        smooIma[i][j-1] = ( receive_upper[j-1] + receive_upper[j] + receive_upper[j+1] + 
                                            subImage[i][j-1] + subImage[i][j] + subImage[i][j+1] +                  //if the upper array is needed
                                            subImage[i+1][j-1] + subImage[i+1][j] + subImage[i+1][j+1])/9;
                        calculated =2;  
                    } else if(i == row_per_process-1&& !(rank ==size-1 && size-1 == pixel)){
                        smooIma[i][j-1] = ( subImage[i-1][j-1] + subImage[i-1][j] + subImage[i-1][j+1] + 
                                            subImage[i][j-1] + subImage[i][j] + subImage[i][j+1] +                  //if the lower array is needed
                                            receive_lower[j-1]  + receive_lower[j]  + receive_lower[j+1])/9;
                    } else {
                        smooIma[i][j-1] = ( subImage[i-1][j-1] + subImage[i-1][j] + subImage[i-1][j+1] + 
                                            subImage[i][j-1] + subImage[i][j] + subImage[i][j+1] +                  //if everything is present
                                            subImage[i+1][j-1] + subImage[i+1][j] + subImage[i+1][j+1])/9;
                        calculated = 5;
                    }
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);            //synchronize all processes before sending and receiving smoothed edges
    /*
        In order to save memory, the sharing of edges is the same as the previous code section.
        There is no extra array created to share edges.
    */
    for (i=0;i<pixel-2;i++){
        send_upper[i] = smooIma[0][i];
        int lower = row_per_process-1;       
        send_lower[i] = smooIma[lower][i];
    }

    if (rank !=0 && rank != size -1){
        MPI_Send(
            &send_lower,        //data pointer
            pixel,                  //count
            MPI_INT,            //type
            rank+1,                  //receiver
            0,                  //tag
            MPI_COMM_WORLD);
    }
    if (rank >1){
        MPI_Recv(&receive_upper,pixel,MPI_INT,rank-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    if (rank >1){
        MPI_Send(&send_upper,pixel,MPI_INT,rank-1,1,MPI_COMM_WORLD);
    }
    if (rank !=0 && rank != size -1){
        MPI_Recv(&receive_lower,pixel,MPI_INT,rank+1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    MPI_Barrier(MPI_COMM_WORLD);            //Synchronize before starting line detecting process
    int line_detected [row_per_process][pixel-4];    //the line detected subImage


    if(rank != 0 ){
        int k;  // we check 4 matrices 
        int line_matrix[3][3];  // line matrices to be convoluted with
        int checker;            // computed convolution value
        for(k=0;k<4;k++){
            if(k==0){
                line_matrix[0][0] = -1; line_matrix[0][1] = -1; line_matrix[0][2] = -1;
                line_matrix[1][0] =  2; line_matrix[1][1] =  2; line_matrix[1][2] =  2;
                line_matrix[2][0] = -1; line_matrix[2][1] = -1; line_matrix[2][2] = -1;
            }
            if(k==1){
                line_matrix[0][0] = -1; line_matrix[0][1] =  2; line_matrix[0][2] = -1;
                line_matrix[1][0] = -1; line_matrix[1][1] =  2; line_matrix[1][2] = -1;
                line_matrix[2][0] = -1; line_matrix[2][1] =  2; line_matrix[2][2] = -1;
            }
            if(k==2){
                line_matrix[0][0] =  2; line_matrix[0][1] = -1; line_matrix[0][2] = -1;
                line_matrix[1][0] = -1; line_matrix[1][1] =  2; line_matrix[1][2] = -1;
                line_matrix[2][0] = -1; line_matrix[2][1] = -1; line_matrix[2][2] =  2;
            }
            if(k==3){
                line_matrix[0][0] = -1; line_matrix[0][1] = -1; line_matrix[0][2] =  2;
                line_matrix[1][0] = -1; line_matrix[1][1] =  2; line_matrix[1][2] = -1;
                line_matrix[2][0] =  2; line_matrix[2][1] = -1; line_matrix[2][2] = -1;
            }
            for (i=0;i<row_per_process;i++){
                if(!((rank == 1 && i < 2)||(rank == size -1 && i > row_per_process-3))) {
                    for (j=1; j<pixel-3; j++){
                        calculated = 16; 
                        //Detects image if every process has one row
                        if(pixel ==  size-1 && !(rank ==1 || rank ==2 || rank == size-1 || rank == size-2)) {
                            if(line_detected[i][j-1] != 255){
                                checker =               receive_upper[j-1] *  line_matrix[0][0] + receive_upper[j]  *  line_matrix[0][1]+ receive_upper[j+1]    *  line_matrix[0][2]+ 
                                                        smooIma[i][j-1]    *  line_matrix[1][0] + smooIma[i][j]     *  line_matrix[1][1]+ smooIma[i][j+1]       *  line_matrix[1][2]+                 
                                                        receive_lower[j-1] *  line_matrix[2][0] + receive_lower[j]  *  line_matrix[2][1]+ receive_lower[j+1]    *  line_matrix[2][2];
                                if (checker > threshold){
                                    line_detected[i][j-1] = 255;
                                } else {
                                    line_detected[i][j-1] = 0;
                                }

                            }
                        //if the upper array is needed
                        } else if(i ==0 && !((rank ==1 || rank == 2 )&& size -1== pixel)){
                            if(line_detected[i][j-1] != 255){
                            checker  =                  receive_upper[j-1] *  line_matrix[0][0]+ receive_upper[j]   *  line_matrix[0][1]+ receive_upper[j+1]    *  line_matrix[0][2]+ 
                                                        smooIma[i][j-1]    *  line_matrix[1][0]+ smooIma[i][j]      *  line_matrix[1][1]+ smooIma[i][j+1]       *  line_matrix[1][2]+                  
                                                        smooIma[i+1][j-1]  *  line_matrix[2][0]+ smooIma[i+1][j]    *  line_matrix[2][1]+ smooIma[i+1][j+1]     *  line_matrix[2][2];
                                if (checker > threshold){
                                    line_detected[i][j-1] = 255;
                                } else {
                                    line_detected[i][j-1] = 0;
                                } 
                            }
                        //if the lower array is needed
                        } else if(i == row_per_process -1 && !((rank ==size-1 ||rank == size -2)&& size-1 == pixel)){
                            if(line_detected[i][j-1] != 255){
                                checker =            smooIma[i-1][j-1]     *  line_matrix[0][0] + smooIma[i-1][j]   *  line_matrix[0][1]+ smooIma[i-1][j+1]     *  line_matrix[0][2]+ 
                                                     smooIma[i][j-1]       *  line_matrix[1][0]+ smooIma[i][j]      *  line_matrix[1][1]+ smooIma[i][j+1]       *  line_matrix[1][2]+                  
                                                     receive_lower[j-1]    *  line_matrix[2][0] + receive_lower[j]  *  line_matrix[2][1]+ receive_lower[j+1]    *  line_matrix[2][2];
                                if (checker > threshold){
                                    line_detected[i][j-1] = 255;
                                } else {
                                    line_detected[i][j-1] = 0;
                                } 
                            }
                        //if everything is present
                        } else {
                            if(line_detected[i][j-1] != 255){
                            checker =                   smooIma[i-1][j-1]  *  line_matrix[0][0]+ smooIma[i-1][j]    *  line_matrix[0][1]+ smooIma[i-1][j+1]     *  line_matrix[0][2]+ 
                                                        smooIma[i][j-1]    *  line_matrix[1][0]+ smooIma[i][j]      *  line_matrix[1][1]+ smooIma[i][j+1]       *  line_matrix[1][2]+                  
                                                        smooIma[i+1][j-1]  *  line_matrix[2][0]+ smooIma[i+1][j]    *  line_matrix[2][1]+ smooIma[i+1][j+1]     *  line_matrix[2][2];
                            calculated = 5;
                                if (checker > threshold){
                                    line_detected[i][j-1] = 255;
                                } else {
                                    line_detected[i][j-1] = 0;
                                }
                            } 
                        }
                    }
                }
            }
        }
    }   
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(line_detected, (pixel-4)*(row_per_process), MPI_INT, calculatedImage, (pixel-4)*row_per_process, MPI_INT, 0, MPI_COMM_WORLD);

    /*
        The gathered arrays are gathered in the calcualted Image array and in this section it is written to the output file.
        The first for loop starts from row_per_process +2 because the array of the process 0 is not filled with data but with zerooes.
    */

    if (rank==0){
        FILE *for_output = fopen(argv[2],"w");
        for (i = row_per_process +2; i < pixel + row_per_process -2; i++){
            for(j = 0; j<pixel-4; j++){
                fprintf(for_output, "%d ", calculatedImage[i][j]);
                //printf("%d ",calculatedImage[i][j]);
            }
            fprintf(for_output, "\n" );
            //  printf("\n");
        }
        fclose(for_output);
    }

/*
    //This code section is to check every process ingredients of arrays.
    //Creates a seperate output file for each process.
    char out[20] = "I";

    for(i=0;i<rank;i++){
        strcat(out,"I");
    }
        //Test output
    FILE *f = fopen(out,"w");
        fprintf(f,"Rank %d The subImage array: \n", rank);
       for (i = 0; i < row_per_process; i++){
            for(j = 0; j<pixel; j++){
                fprintf(f, "%d ", subImage[i][j]);
            }
            fprintf(f, "\n" );
        }
       fprintf(f,"The smooImage array(calculated = %d):\n",calculated);

        for (i = 0; i < row_per_process; i++){
            for(j = 0; j<pixel-2; j++){
                fprintf(f, "%d ", smooIma[i][j]);
            }
            fprintf(f, "\n" );
        }
        
        fprintf(f,"The line_detected array(calculated = %d):\n",calculated);

        for (i = 0; i < row_per_process; i++){
            for(j = 0; j<pixel-4; j++){
                fprintf(f, "%d ", line_detected[i][j]);
            }
            fprintf(f, "\n" );
        }



        fprintf(f,"Received Lower:\n");

        for (i=0; i<pixel;i++){
            fprintf(f,"%d ",receive_lower[i]);
        }
            fprintf(f, "\n" );
        fprintf(f,"Received Upper:\n");

        for (i=0; i<pixel;i++){
            fprintf(f,"%d ",receive_upper[i]);
        }
            fprintf(f, "\n" );
        fprintf(f,"Send Lower:\n");

        for (i=0; i<pixel;i++){
            fprintf(f,"%d ",send_lower[i]);
        }
            fprintf(f, "\n" );
        fprintf(f,"Send Upper:\n");

        for (i=0; i<pixel;i++){
            fprintf(f,"%d ",send_upper[i]);
        }
            fprintf(f, "\n" );
    fclose(f);


*/

    MPI_Finalize();


    return 0;
}
