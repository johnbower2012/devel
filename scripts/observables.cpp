#include "observables.hpp"

double median_Width(arma::mat function, double factor){
  int rows = function.n_rows;
  int cols = function.n_cols;
  double median_width, sum=0.0;
  arma::vec tally = arma::zeros<arma::vec>(rows-1);
  for(int i=0;i<rows-1;i++){
    tally(i) = (function(i+1,1) + function(i,1))*(function(i+1,0) - function(i,0))/2.0;
    sum += tally(i);
    tally(i) = sum;
  }
  sum*=factor;
  for(int i=0;i<rows-1;i++){
    if(tally(i)>sum){
      median_width = (function(i+1,0) + function(i,0))/2.0;
      break;
    }
  }
  return median_width;
}

int main(int argc, char* argv[]){
  
