#include<iostream>
#include<random>
#include<cmath>

int main(int argc, char* argv[]){
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::normal_distribution<double> distribution(7.3,1.5);

  double mean=0.0,std=0.0,x=0.0,mean2=0.0,std2=0.0;
  int steps=100000;
  for(int i=0;i<steps;i++){
    x = distribution(generator);
    mean += x;
    std += x*x;
    mean2 += x*x;
    std2 += x*x*x*x;
    //    std += (x-mean/(double) (i+1))*(x-mean/(double) (i+1));
  }
  mean /= (double) steps;
  std /= (double) steps;
  mean2 /= (double) steps;
  std2 /= (double) steps;
  std -= mean*mean;
  std2 -= mean2*mean2;
  std = sqrt(std);
  std2 = sqrt(std2);
  printf("%f +/- %f\n",mean,std);
  printf("%f +/- %f\n",mean2,std2);

  return 0;
}