// File:    mmsp2xyz.cpp
// Purpose: reads MMSP grid containing sparse floats, tracks specified grain
// Output:  CSV file specifying XYZ+phase
// Depends: MMSP, zlib

// Questions/Comments to trevor.keller@gmail.com (Trevor Keller)
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <zlib.h>
#include <sstream>
#include <cmath>
#include <map>
#include "MMSP.hpp"

using namespace MMSP;

const int hashsize = 200;

int hashfunc(long long num) {
  long long hashvalue = 0;
  long long power = 1;
  while (num > 0) {
    int digit = num % 10;
    num /= 10;
    hashvalue += digit * power;
    power *= 33;
    hashvalue %= hashsize;
  }
  return hashvalue;
}

int main(int argc, char* argv[]) {
	if ( argc > 4 || argc < 3 ) {
		std::cout << "Usage: " << argv[0] << " data.dat grain_id output.csv\n";
    std::cout << "Or (for scalar int): " << "Usage: " << argv[0] << " data.dat output.csv\n";
		return ( 1 );
	}

	// file open error check
	std::ifstream input(argv[1]);
	if (!input) {
		std::cerr << "File input error: could not open " << argv[1] << ".\n\n";
		exit(-1);
	}

	// read data type
	std::string type;
	getline(input, type, '\n');

	bool int_type = (type.find("int") != std::string::npos);
	bool unsigned_int_type = (type.find("unsigned int") != std::string::npos);
	bool long_type = (type.find("long") != std::string::npos && type.find("unsigned long") == std::string::npos);
	bool unsigned_long_type = (type.find("unsigned long") != std::string::npos);
	bool short_type = (type.find("short") != std::string::npos);
	bool unsigned_short_type = (type.find("unsigned short") != std::string::npos);
	bool float_type = (type.find("float") != std::string::npos);
	bool double_type = (type.find("double") != std::string::npos);
	bool long_double_type = (type.find("long double") != std::string::npos);
	bool scalar_type = (type.find("scalar") != std::string::npos);
	bool vector_type = (type.find("vector") != std::string::npos);
	bool sparse_type = (type.find("sparse") != std::string::npos);

  if (not sparse_type and not vector_type) {  
	  // generate output file name
	  std::stringstream filename;
	  if (argc == 3)
		  filename << argv[2];
	  else{
		  std::cout << "Usage: " << argv[0] << " data.dat output.csv\n";
		  return ( 1 );
	  }
      

	  std::ofstream output(filename.str().c_str());
	  if (!output) {
		  std::cerr << "File output error: could not open ";
		  std::cerr << filename.str() << "." << std::endl;
		  exit(-1);
	  }

  	// read grid dimension
	  int dim;
	  input >> dim;
	  #ifdef DEBUG
	  std::cout<<"Grid is "<<dim<<"-dimensional."<<std::endl;
	  #endif

	  // read number of fields
	  int fields;
	  input >> fields;
	  #ifdef DEBUG
	  std::cout<<"Grid has "<<fields<<" fields."<<std::endl;
	  #endif

	  // read grid sizes
	  int g0[3] = {0, 0, 0};
	  int g1[3] = {0, 0, 0};
	  for (int i = 0; i < dim; i++)
		  input >> g0[i] >> g1[i];
	  #ifdef DEBUG
	  std::cout<<"Grid edge is "<<g1[0] - g0[0]<<std::endl;
	  #endif

	  // read cell spacing
	  float dx[3] = {1.0, 1.0, 1.0};
	  for (int i = 0; i < dim; i++)
		  input >> dx[i];
	  #ifdef DEBUG
	  std::cout<<"Grid spacing is "<<dx[0]<<std::endl;
	  #endif

	  // ignore trailing endlines
	  input.ignore(10, '\n');


	  // determine byte order
	  std::string byte_order;
	  if (0x01 & static_cast<int>(1)) byte_order = "LittleEndian";
	  else byte_order = "BigEndian";
	  #ifdef DEBUG
	  std::cout<<"Grid is "<<byte_order<<std::endl;
	  #endif


	// Estimate number of grains, for color randomization
/*
	int est_grains = 10000;
	if (dim==2) est_grains=static_cast<int>(1.5*float((g1[0]-g0[0])*(g1[1]-g0[1]))/(M_PI*10.*10.)); // average grain is a disk of radius 10
	else if (dim==3) est_grains=static_cast<int>(1.5*float((g1[0]-g0[0])*(g1[1]-g0[1])*(g1[2]-g0[2]))/(4./3*M_PI*10.*10.*10.)); // Average grain is a sphere of radius 10 voxels
	#ifdef DEBUG
	std::cout<<"Grid contains approx. "<<est_grains<<" grains."<<std::endl;
	#endif
	std::vector<int> colors;
	for (unsigned int i=0; i<est_grains; i++)
		colors.push_back(rand() % est_grains);
*/
	  // read number of blocks
	  int blocks;
	  input.read(reinterpret_cast<char*>(&blocks), sizeof(blocks));
    std::vector<unsigned long>* grain_sizes = new std::vector<unsigned long>;

    double number_of_pixels=0.0;
    unsigned long number_of_grains = 0;
    unsigned long max_grain_id=0;
	  for (int i = 0; i < blocks; i++) {
		  #ifdef DEBUG
		  std::cout<<"  Reading block "<<i+1<<" of "<<blocks<<std::endl;
		  #endif
		  // read block limits
		  int lmin[3] = {0, 0, 0};
		  int lmax[3] = {0, 0, 0};
		  for (int j = 0; j < dim; j++) {
			  input.read(reinterpret_cast<char*>(&lmin[j]), sizeof(lmin[j]));
			  input.read(reinterpret_cast<char*>(&lmax[j]), sizeof(lmax[j]));
		  }
		  #ifdef DEBUG
		  std::cout<<"  Block edge is "<<lmax[0] - lmin[0]<<std::endl;
		  #endif
		  int blo[dim];
      int bhi[dim];
      // read boundary conditions
      for (int j = 0; j < dim; j++) {
        input.read(reinterpret_cast<char*>(&blo[j]), sizeof(blo[j]));
        input.read(reinterpret_cast<char*>(&bhi[j]), sizeof(bhi[j]));
      }

		  // read grid data
		  unsigned long size, rawSize;
		  input.read(reinterpret_cast<char*>(&rawSize), sizeof(rawSize)); // read raw size
		  input.read(reinterpret_cast<char*>(&size), sizeof(size)); // read compressed size
		  char* compressed_buffer = new char[size];
		  input.read(compressed_buffer, size);
		  #ifdef DEBUG
		  std::cout<<"  Read "<<size<<" B, compressed data."<<std::endl;
		  #endif
		  char* buffer;
  		if (size!=rawSize) {
			  // Decompress data
			  buffer = new char[rawSize];
			  int status;
			  status = uncompress(reinterpret_cast<unsigned char*>(buffer), &rawSize, reinterpret_cast<unsigned char*>(compressed_buffer), size);
			  switch( status ) {
			  case Z_OK:
				  break;
			  case Z_MEM_ERROR:
				  std::cerr << "Uncompress: out of memory." << std::endl;
				  exit(1);
				  break;
			  case Z_BUF_ERROR:
				  std::cerr << "Uncompress: output buffer wasn't large enough." << std::endl;
				  exit(1);
				  break;
			  }
			  delete [] compressed_buffer;
		  } else {
			  buffer=compressed_buffer;
			  compressed_buffer=NULL;
		  }

			if (int_type) {
        if (dim == 2) {
					MMSP::grid<2,MMSP::scalar<int> > GRID(fields, lmin, lmax);
				  GRID.from_buffer(buffer);
				  for (int k = 0; k < (lmax[1]-lmin[1]); k++){
            for (int l = 0; l < (lmax[0]-lmin[0]); l++){
              vector<int> x (2,0);
              x[0] = lmin[0] + l;
              x[1] = lmin[1] + k;
//              t << x[0] <<" "<< x[1] <<" "<< colors[GRID(x)%est_grains] <<"\n";
//std::cout<< x[0] << " " << x[1] << "\n";
              output << x[0] << " " << x[1] << " " << hashfunc(GRID(x)) << "\n";  //no color randomization
//std::cout<< x[0] << " " << x[1] << "\n";
//std::cout<< "GRID(x) "<<GRID(x) << "\n";
              if(max_grain_id<GRID(x)){
                max_grain_id=GRID(x);
//std::cout<< "max_grain_id "<<max_grain_id << "\n";
                (*grain_sizes).resize(max_grain_id+1, 0);
              }
              (*grain_sizes)[GRID(x)]+=1;
//std::cout<< "12314" << "\n";
            } // for l
				  } // for k 	
				} else if (dim == 3) {
		  	  MMSP::grid<3,MMSP::scalar<int> > GRID(fields, lmin, lmax);
				  GRID.from_buffer(buffer);
          for (int l = 0; l < (lmax[0]-lmin[0]); l++){
				    for (int k = 0; k < (lmax[1]-lmin[1]); k++){
              for (int m = 0; m < (lmax[2]-lmin[2]); m++){
                vector<int> x (3,0);
                x[0] = lmin[0] + l;
                x[1] = lmin[1] + k;
                x[2] = lmin[2] + m;
//						  output << colors[GRID(k)%est_grains] << " ";
                output << x[0] <<" "<< x[1] <<" "<< x[2] <<" "<< hashfunc(GRID(x)) << "\n";  //no color randomization
                if(max_grain_id<GRID(x)){
                  max_grain_id=GRID(x);
                  (*grain_sizes).resize(max_grain_id+1, 0);
                }
                (*grain_sizes)[GRID(x)]+=1;
              } // for m
            } // for k
          }// for l 
				}// if dim == 3
        int local_number_of_pixels = 1;
        for(int j=0; j<dim; j++) {
          local_number_of_pixels *= (lmax[j]-lmin[j]);
        }
        number_of_pixels += (double)local_number_of_pixels;
      }//if int_type

			if (unsigned_long_type) {
        if (dim == 2) {
					MMSP::grid<2,MMSP::scalar<unsigned long> > GRID(fields, lmin, lmax);
				  GRID.from_buffer(buffer);
				  for (int k = 0; k < (lmax[1]-lmin[1]); k++){
            for (int l = 0; l < (lmax[0]-lmin[0]); l++){
              vector<int> x (2,0);
              x[0] = lmin[0] + l;
              x[1] = lmin[1] + k;
//              output << x[0] <<" "<< x[1] <<" "<< colors[GRID(x)%est_grains] <<"\n";
//std::cout<< x[0] << " " << x[1] << "\n";
              output << x[0] << " " << x[1] << " " << hashfunc(GRID(x)) << "\n";  //no color randomization
//std::cout<< x[0] << " " << x[1] << "\n";
//std::cout<< "GRID(x) "<<GRID(x) << "\n";
              if(max_grain_id<GRID(x)){
                max_grain_id=GRID(x);
                (*grain_sizes).resize(max_grain_id+1, 0);
              }
              (*grain_sizes)[GRID(x)]+=1;
            } // for l
				  } // for k 	
				} else if (dim == 3) {
		  	  MMSP::grid<3,MMSP::scalar<unsigned long> > GRID(fields, lmin, lmax);
				  GRID.from_buffer(buffer);
          for (int l = 0; l < (lmax[0]-lmin[0]); l++){
				    for (int k = 0; k < (lmax[1]-lmin[1]); k++){
              for (int m = 0; m < (lmax[2]-lmin[2]); m++){
                vector<int> x (3,0);
                x[0] = lmin[0] + l;
                x[1] = lmin[1] + k;
                x[2] = lmin[2] + m;
//						    output << colors[GRID(k)%est_grains] << " ";
                output << x[0] <<" "<< x[1] <<" "<< x[2] <<" "<< hashfunc(GRID(x)) << "\n";  //no color randomization
                if(max_grain_id<GRID(x)){
                  max_grain_id=GRID(x);
                  (*grain_sizes).resize(max_grain_id+1, 0);
                }
                (*grain_sizes)[GRID(x)]+=1;
              } // for m
            } // for k
          }// for l 
				}// if dim == 3
        unsigned long local_number_of_pixels = 1;
        for(int j=0; j<dim; j++) {
          local_number_of_pixels *= (lmax[j]-lmin[j]);
        }
        number_of_pixels += (double)local_number_of_pixels;
      }//if unsigned_long_type
		  // clean up
		  delete [] buffer; 
		}// for int i //loop over blocks
    output.close();
    double average_grain_size = 0.0;
    //    std::ofstream ofs("grain_size.txt", std::ios::out);
    for(int i=0; i<(*grain_sizes).size(); i++){
      if(((*grain_sizes)[i]!=0)){
        number_of_grains += 1;
	//    ofs<<(*grain_sizes)[i]<<std::endl;
      }
    }
    average_grain_size = 2*sqrt(number_of_pixels/number_of_grains/3.14/2*3);
    std::cout<<"average_grain_diameter is  "<<average_grain_size<<std::endl;
    std::cout<<"number of grains is  "<<number_of_grains<<std::endl;
//    std::ofstream ofs("grain_size.txt", std::ios::out|std::ios::app);
//    ofs<<average_grain_size<<std::endl;
    //ofs.close();

    (*grain_sizes).clear();

  }// if (not sparse_type and not vector_type)
  else{
	if ( argc != 4 ) {
		std::cout << "Usage: " << argv[0] << " data.dat grain_id output.csv\n";
		return ( 1 );
	}

	// file open error check
	std::ifstream input(argv[1]);
	if (!input) {
		std::cerr << "File input error: could not open " << argv[1] << ".\n\n";
		exit(-1);
	}

	const int id(atoi(argv[2]));
	if (id == 0) {
		std::cerr << "Input warning: " << argv[2] << " does not appear to be an integer." << std::endl;
	}

	// grid type error check: read line, "grid:sparse:float"
	if (type.substr(0, 4) != "grid") {
		std::cerr << "File input error: file does not contain grid data." << std::endl;
		exit(-1);
	} else if (type.substr(5, 6) != "sparse") {
		std::cerr << "File input error: grid does not contain sparse data." << std::endl;
		exit(-1);
	} else if (type.substr(12, 5) != "float") {
		std::cerr << "File input error: vector data does not contain floats." << std::endl;
		exit(-1);
	} 

	// read grid dimension
	int dim;
	input >> dim;

	std::vector<MMSP::vector<int> > points;
	std::vector<float> weights;

	if (dim == 2) {
		std::cout << "XYZ implies 3D data." << std::endl;
		exit(1);
	} else if (dim == 3) {
		// construct grid object
		MMSP::grid<3, MMSP::sparse<float> > grid(argv[1]);
		for (int d = 0; d < dim; ++d) dx(grid, d) = 0.375;

		// Populate the image from MMSP data.
		MMSP::vector<int> min(3, 500); // origin of grain
		for (int n = 0; n < nodes(grid); ++n) {
			MMSP::vector<int> x = position(grid, n);
			int S = length(grid(n));
			for (int s = 0; s < S; ++s) {
				if (MMSP::index(grid(n), s) == id) {
					points.push_back(x);
					weights.push_back(grid(n)[id]);
					for (int d = 0; d < 3; ++d) if (x[d] < min[d]) min[d] = x[d];
				}
			}
		}
		if (points.size() != weights.size()) {
			std::cout << "Error: XYZ vs weight size mismatch." << std::endl;
			exit(1);
		}
		std::ofstream output(argv[3]);
		if (!output) {
			std::cerr << "File output error: could not create " << argv[3] << ".\n\n";
			exit(-1);
		}
		output << "#X,Y,Z,nx,ny,nz\n";
		for (int i = 0; i < points.size(); ++i) {
			MMSP::vector<MMSP::sparse<float> > normal = MMSP::gradient(grid, points[i]);
			for (int d = 0; d < 3; ++d) output << dx(grid, d)*double(points[i][d] - min[d]) << '\t';
			for (int d = 0; d < 3; ++d) {
				output << dx(grid, d)*double(normal[d][id]);
				if (d < dim - 1) output << '\t';
				else output << '\n';
			}
		}
	} else {
		std::cerr << "Error: " << dim << "-D data is not supported!" << std::endl;
		exit(1);
	}
  }

	return 0;
}
