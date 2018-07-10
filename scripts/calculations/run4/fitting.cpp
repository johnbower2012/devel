#include<iostream>
#include<iomanip>
#include<random>
#include<chrono>
#include<armadillo>
#include<fstream>

std::ofstream ofile;
std::ifstream ifile;

arma::vec linear_regression_ls(arma::vec y, arma::mat X){
  int points = y.n_elem;
  arma::mat temp = X.t()*X;
  arma::vec beta = temp.i()*X.t()*y;
  return beta;
}
arma::mat linear_regression_ls(arma::mat Y, arma::mat X){
  int points = Y.n_rows;
  int parameters = X.n_cols-1;
  int observables = Y.n_cols;
  arma::mat temp = arma::zeros<arma::mat>(points,2);

  arma::mat Beta = arma::zeros<arma::mat>(parameters*observables,2);
  arma::vec beta = arma::zeros<arma::vec>(2);
  arma::mat X_ = arma::zeros<arma::mat>(points,2);
  arma::vec y = arma::zeros<arma::vec>(points);

  /*
  for(int i=0;i<parameters;i++){
    for(int j=0;j<points;j++){
      X_(j,0) = 1.0;
      X_(j,1) = X(j,i+1);
    }
    temp = X_.t()*X_;
    temp = temp.i();
    for(int j=0;j<observables;j++){
      y = Y.col(j);
      beta = temp*X_.t()*y;
      Beta.row(i*observables+j) = beta.t();
    }
  }
  */
  for(int i=0;i<observables;i++){
    y = Y.col(i);
    for(int j=0;j<parameters;j++){
      for(int k=0;k<points;k++){
	X_(k,0) = 1.0;
	X_(k,1) = X(k,j+1);
      }
      temp = X_.t()*X_;
      temp = temp.i();
      beta = temp*X_.t()*y;
      Beta.row(i*parameters+j) = beta.t();
    }
  }

  return Beta;
}

int main(int argc, char* argv[]){
  int points=1000,dimensions=4,observables=12;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_real_distribution<double> distribution(-0.5,0.5);
  arma::mat X = arma::zeros<arma::mat>(points,dimensions+1);
  arma::mat Y = arma::zeros<arma::mat>(points,observables);
  arma::vec y = arma::zeros<arma::vec>(points);
  arma::vec result = arma::zeros<arma::vec>(points);
  arma::vec beta = arma::zeros<arma::vec>(points);

  ifile.open("plot.dat");
  for(int i=0;i<points;i++){
    X(i,0) = 1.0;
    for(int j=0;j<dimensions;j++){
      ifile >> X(i,1+j);
    }
    for(int j=0;j<observables;j++){
      ifile >> Y(i,j);
    }
    y(i) = Y(i,0);
  }

  arma::mat Beta = linear_regression_ls(Y,X);

  beta = linear_regression_ls(y,X);
  beta.print("beta");
  
  ofile.open("beta.dat");
  ofile << "#parameters(1,2,3...)observables(1,2,3...)" << std::endl;
  for(int i=0;i<dimensions*observables;i++){
    for(int j=0;j<2;j++){
      ofile << " " << Beta(i,j);
    }
    ofile << std::endl;
  }
  ofile.close();

  return 0;
}
