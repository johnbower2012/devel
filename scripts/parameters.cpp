#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include "armadillo"

void print_file(std::string outfilename, arma::mat matrix);
arma::mat load_range_file(std::string filename, int parameters, std::vector<std::string> &Names);
arma::mat construct_latinhypercube_sampling(int samples, arma::mat range);
void create_parameter_prior_files(std::vector<std::string> Names, arma::mat hypercube);

int main(int argc, char* argv[]){
  std::string filename;
  std::string printname = "moments_parameters.dat";
  int lines,lhp_samples;
  if(argc!=4){
    std::cout << "Usage: Enter also 'fn lines lhp_s' the same line.\n";
    exit(1);
  }
  else{
    filename=argv[1];
    lines=atoi(argv[2]);
    lhp_samples=atoi(argv[3]);
  }

  /*Load parameter_priors information from file
    We store twice in Names[i] to rid outselves of the first field
    in the file, "UNIFORM"
   */
  arma::mat File = arma::zeros<arma::mat>(lines,2);
  std::vector<std::string> Names(lines);
  File = load_range_file(filename,lines,Names);

  /*Construct the generic hypercube sampling in terms of an int list, 0 to lhp_samples
    We end with a lines by lhp_s matrix. Then construct full numerical lhp sampling 
    using the input from file and the previous generic sampling matrix
  */
  arma::mat hypercube = arma::zeros<arma::mat>(lhp_samples,lines);
  hypercube = construct_latinhypercube_sampling(lhp_samples,File);
  print_file(printname,hypercube);

  /*Construct the parameter files for each sampling 
   */
  create_parameter_prior_files(Names, hypercube);

  return 0;
}


void print_file(std::string outfilename, arma::mat matrix){
  int rows = matrix.n_rows;
  int cols = matrix.n_cols;
  std::ofstream ofile;
  ofile.open(outfilename);
  for(int i=0;i<rows;i++){
    for(int j=0;j<cols;j++){
      ofile << ' ' << matrix(i,j);
    }
    ofile << '\n';
  }
  ofile.close();
}
arma::mat load_range_file(std::string filename, int parameters, std::vector<std::string> &Names){
  std::ifstream ifile;
  arma::mat File = arma::zeros<arma::mat>(parameters,2);
  Names.resize(parameters);
  ifile.open(filename);
  for(int i=0;i<parameters;i++){
    ifile >> Names[i]; ifile >> Names[i];
    ifile >> File(i,0);
    ifile >> File(i,1);
  }
  ifile.close();
  return File;
}
arma::mat construct_latinhypercube_sampling(int samples, arma::mat range){
  int parameters = range.n_rows;
  arma::mat hypercube = arma::zeros<arma::mat>(samples,parameters);
  arma::vec hyperlist = arma::linspace<arma::vec>(0,samples-1,samples);
  for(int i=0;i<parameters;i++){
    hyperlist=shuffle(hyperlist);
    hypercube.col(i) = hyperlist;
  }

  float init,final,dx;
  for(int i=0;i<parameters;i++){
    init = range(i,0);
    final = range(i,1);
    dx = (final-init)/(samples-1);
    hyperlist = hypercube.col(i);
    hyperlist = init + dx*hyperlist;
    hypercube.col(i) = hyperlist;
  }
  return hypercube;
}  
void create_parameter_prior_files(std::vector<std::string> Names, arma::mat hypercube){
  std::ofstream ofile;
  int lhp_samples = hypercube.n_rows;
  int parameters = hypercube.n_cols;
  for(int i=0;i<lhp_samples;i++){
    char fn[50];
    sprintf(fn,"../model_output/run%04d/parameters.dat",i);
    ofile.open(fn);
    for(int j=0;j<parameters;j++){
      ofile << Names[j] << " " << hypercube(i,j) << '\n';
    }
    ofile.close();
  }
}
