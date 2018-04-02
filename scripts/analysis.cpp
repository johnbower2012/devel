#include<iostream>
#include<cmath>
#include<iomanip>
#include "armadillo"

void tilde_function(const arma::mat &matrix, const arma::vec &error, arma::vec &mean, arma::mat &tilde){
  int repetitions = matrix.n_rows;
  int observables = matrix.n_cols;
  mean = arma::zeros<arma::vec>(observables);
  tilde = arma::zeros<arma::mat>(repetitions,observables);

  for(int i=0;i<observables;i++){
    for(int j=0;j<repetitions;j++){
      mean(i) += matrix(j,i);
    }
    mean(i) /= (double) repetitions;
  }
  for(int i=0;i<repetitions;i++){
    for(int j=0;j<observables;j++){
      tilde(i,j) = (matrix(i,j) - mean(j))/(error(j)*mean(j));
    }
  }
}
void covariance_function(const arma::mat &matrix, arma::mat &covariance){
  int repetitions = matrix.n_rows;
  int observables = matrix.n_cols;
  covariance = arma::zeros<arma::mat>(observables,observables);

  for(int i=0;i<observables;i++){
    for(int j=0;j<observables;j++){
      for(int k=0;k<repetitions;k++){
	covariance(i,j) += matrix(k,i)*matrix(k,j);
      }
      covariance(i,j) /= (double) repetitions;
    }
  }
}
  

int main(int argc, char* argv[]){
  arma::mat matrix = arma::zeros<arma::mat>(10,5);
  arma::mat tilde = arma::zeros<arma::mat>(10,5);
  arma::mat covariance = arma::zeros<arma::mat>(5,5);
  arma::vec mean = arma::zeros<arma::vec>(5);
  arma::vec error = arma::zeros<arma::vec>(5);
  for(int i=0;i<5;i++){
    for(int j=0;j<10;j++){
      matrix(j,i) = i+j;
    }
    error(i) = 0.10;
  }
  tilde_function(matrix,error,mean,tilde);
  covariance_function(tilde,covariance);
  matrix.print();
  std::cout << '\n';
  mean.print();
  std::cout << '\n';
  error.print();
  std::cout << '\n';
  tilde.print();
  std::cout << '\n';
  covariance.print();

  
  return 0;
}