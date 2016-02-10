// Calculates the discrete costate fluxes
#include<iostream>
#include<math.h>
#include"quasiOneD.h"
#include<vector>
#include<Eigen/Eigen>
//#include<Eigen/Dense>
//#include<Eigen/Sparse>
//#include <Eigen/IterativeLinearSolvers>
//#include<Eigen/SparseLU>
//#include<Eigen/LU>
#include <stdio.h>
#include <iomanip>
#include "globals.h"
#include "flovar.h"
#include "convert.h"
#include "flux.h"

using namespace Eigen;

MatrixXd evaldRdS(std::vector <double> Flux, std::vector <double> S,
                  std::vector <double> W);

void JacobianCenter(std::vector <double> &J,
                    double u, double c);

SparseMatrix<double> buildAMatrix(std::vector <double> Ap,
                                  std::vector <double> An,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min);

SparseMatrix<double> buildAFD(std::vector <double> W,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min);
SparseMatrix<double> buildAFD2(std::vector <double> W,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min);

VectorXd buildbMatrix(std::vector <double> dIcdW);

void StegerJac(std::vector <double> W,
               std::vector <double> &Ap_list,
               std::vector <double> &An_list,
               std::vector <double> &Flux);

void ScalarJac(std::vector <double> W,
               std::vector <double> &Ap_list,
               std::vector <double> &An_list,
               std::vector <double> &Flux);

void BCJac(std::vector <double> W,
           std::vector <double> dt,
           std::vector <double> dx,
           std::vector <double> ddtdW,
           std::vector <double> &dBidWi,
           std::vector <double> &dBidWd,
           std::vector <double> &dBodWd,
           std::vector <double> &dBodWo,
           std::vector <double> &B1,
           std::vector <double> &BN);

void evaldIcdW(std::vector <double> &dIcdW,
               std::vector <double> W,
               std::vector <double> S);

void evaldQdW(std::vector <double> &dQdW,
                   std::vector <double> W,
                   std::vector <double> S);

void evalddtdW(std::vector <double> &ddtdW,
                 std::vector <double> rho,
                 std::vector <double> u,
                 std::vector <double> p,
                 std::vector <double> c,
                 std::vector <double> dx,
                 std::vector <double> S);

std::vector <double> adjoint(std::vector <double> x, 
             std::vector <double> dx, 
             std::vector <double> S,
             std::vector <double> W,
             std::vector <double> &psi,
             std::vector <double> designVar)
{
    //Get Primitive Variables
    std::vector <double> rho(nx), u(nx), e(nx);
    std::vector <double> T(nx), p(nx), c(nx), Mach(nx);
    WtoP(W, rho, u, e, p, c, T); 
    
    // Evalutate dt and d(dt)dW
    std::vector <double> dt(nx, 1), ddtdW(nx, 0);

    // Evaluate dQdW
    std::vector <double> dQdW(3 * nx, 0);
    evaldQdW(dQdW, W, S);

    // Get Jacobians and Fluxes
    std::vector <double> Ap_list(nx * 3 * 3, 0), An_list(nx * 3 * 3, 0);
    std::vector <double> Flux(3 * (nx + 1), 0);
    if(FluxScheme == 0) StegerJac(W, Ap_list, An_list, Flux);
    if(FluxScheme == 1) ScalarJac(W, Ap_list, An_list, Flux);
        
    // Transposed Boundary Flux Jacobians
    std::vector <double> dBidWi(3 * 3, 0);
    std::vector <double> dBidWd(3 * 3, 0);
    std::vector <double> dBodWd(3 * 3, 0);
    std::vector <double> dBodWo(3 * 3, 0);
    std::vector <double> B1(3, 0);
    std::vector <double> BN(3, 0);
    BCJac(W, dt, dx, ddtdW, dBidWi, dBidWd, dBodWd, dBodWo, B1, BN);

    // Build A matrix
    SparseMatrix <double> matA, matAt;
    SparseMatrix <double> matAFD, matAFD2;
    matAt = buildAMatrix(Ap_list, An_list, dBidWi, dBidWd,
                        dBodWd, dBodWo, dQdW, dx, dt, S, u[0]/c[0]);
    matAFD = buildAFD(W, dBidWi, dBidWd, dBodWd, dBodWo, dQdW, dx, dt, S, u[0]/c[0]);
    matAFD2 = buildAFD2(W, dBidWi, dBidWd, dBodWd, dBodWo, dQdW, dx, dt, S, u[0]/c[0]);
    matA = matAt.transpose();
    matA = matAFD2.transpose();
    std::cout.precision(17);
//  std::cout<<matAt<<std::endl;
//  std::cout<<matAFD2<<std::endl;
//  std::cout<<"(matAFD - matAFD2).norm() / matAFD.norm():"<<std::endl;
//  std::cout<<(matAFD - matAFD2).norm() / matAFD.norm()<<std::endl;
//  std::cout<<"(matAFD - matAt).norm() / matA.norm():"<<std::endl;
//  std::cout<<(matAFD - matAt).norm() / matAFD.norm()<<std::endl;
//  std::cout<<"(matAFD2 - matAt).norm() / matA.norm():"<<std::endl;
//  std::cout<<(matAFD2 - matAt).norm() / matAFD2.norm()<<std::endl;

    // Evaluate dIcdW
    std::vector <double> dIcdW(3 * nx, 0);
    evaldIcdW(dIcdW, W, dx);

    // Build B matrix
    VectorXd bvec(3 * nx);
    bvec.setZero();

    bvec = buildbMatrix(dIcdW);
    std::cout<<"Vector B"<<std::endl;
    std::cout<<bvec<<std::endl;

    MatrixXd matAdense(3 * nx, 3 * nx);
    MatrixXd eye(3 * nx, 3 * nx);
    eye.setIdentity();
    matAdense = matA * eye;

    VectorXd xvec(3 * nx);
    xvec.setZero();
    double eig_solv = 0;
    if(eig_solv == 0)
    {
    // Setup Solver and Factorize A
        SparseLU <SparseMatrix <double>, COLAMDOrdering< int > > slusolver;
        slusolver.analyzePattern(matA);
        slusolver.factorize(matA);
        
//      if(slusolver.info() == 0)
//          std::cout<<"Factorization success"<<std::endl;
//      else
//          std::cout<<"Factorization failed. Error: "<<slusolver.info()<<std::endl;
    
        // Solve for X
        xvec = slusolver.solve(bvec);
//      std::cout<<"Adjoint Result:"<<std::endl;
//      std::cout<<xvec<<std::endl;
    }
    if(eig_solv == 1)
    {
        // Full Pivoting LU Factorization
        xvec = matAdense.fullPivLu().solve(bvec);
        std::cout<<"Adjoint Result:"<<std::endl;
        std::cout<<xvec<<std::endl;
        std::cout<<"Identity"<<std::endl;
        std::cout<<(matAdense.inverse() * matAdense).norm()<<std::endl;
    }
    if(eig_solv == 2)
    {
        BiCGSTAB<SparseMatrix <double> > itsolver;
        itsolver.compute(matA);
        if(itsolver.info() == 0)
            std::cout<<"Iterative Factorization success"<<std::endl;
        else
            std::cout<<"Factorization failed. Error: "<<itsolver.info()<<std::endl;
        std::cout << "#iterations:     " << itsolver.iterations() << std::endl;
        std::cout << "estimated error: " << itsolver.error()      << std::endl;
        xvec = itsolver.solve(bvec);
        std::cout<<"Adjoint Result:"<<std::endl;
        std::cout<<xvec<<std::endl;
    }
//  std::cout<<"Condition Number"<<std::endl;
//  std::cout<<matAdense.inverse().norm() * matAdense.norm()<<std::endl;
    JacobiSVD<MatrixXd> svd(matAdense);
    double cond = svd.singularValues()(0) 
    / svd.singularValues()(svd.singularValues().size()-1);
    std::cout<<"Condition Number SVD"<<std::endl;
    std::cout<<cond<<std::endl;
    std::cout<<"||Ax - b||"<<std::endl;
    std::cout<<(matAdense * xvec - bvec).norm()<<std::endl;

    if(u[0] > c[0])
    {
        xvec(0) = xvec(3);
        xvec(1) = xvec(4);
        xvec(2) = xvec(5);
    }

    std::cout<<"Adjoint Result:"<<std::endl;
    std::cout<<xvec<<std::endl;

    // Print out Adjoint 
    FILE *Results;
    Results = fopen("Adjoint.dat", "w");
    fprintf(Results, "%d\n", nx);
    for(int k = 0; k < 3; k++)
    for(int i = 0; i < nx; i++)
        fprintf(Results, "%.15f\n", xvec(i * 3 + k));

    fclose(Results);

    // Evaluate dIcdS
    VectorXd dIcdS(nx + 1);
    dIcdS.setZero();

    // Evaluate psi * dRdS
    VectorXd psidRdS(nx + 1);
    psidRdS.setZero();
    for(int i = 1; i < nx - 1; i++)
    for(int k = 0; k < 3; k++)
    {
        psidRdS(i) += xvec((i - 1) * 3 + k) * Flux[i * 3 + k];
        psidRdS(i) -= xvec(i * 3 + k) * Flux[i * 3 + k];
        if(k == 1)
        {
            psidRdS(i) -= xvec((i - 1) * 3 + k) * p[i - 1];
            psidRdS(i) += xvec(i * 3 + k) * p[i];
        }
    }


    for(int k = 0; k < 3; k++)
    {
        // Cell 0 Inlet is not a function of the shape

        // Cell 1
        psidRdS(1) -= xvec(1 * 3 + k) * Flux[1 * 3 + k];

        // Cell nx - 1
        psidRdS(nx - 1) += xvec((nx - 2) * 3 + k) * Flux[(nx - 1) * 3 + k];

        // Cell nx Outlet is not a function of the shape

        if(k == 1)
        {
            psidRdS(1) += xvec(1 * 3 + k) * p[1];
            psidRdS(nx - 1) -= xvec((nx - 2) * 3 + k) * p[nx - 1];
        }
    }

    std::cout<<"psidRdS:"<<std::endl;
    for(int i = 0; i < nx + 1; i++)
        std::cout<<psidRdS(i)<<std::endl;
    
    MatrixXd dRdS(3 * nx, nx + 1);
    dRdS = evaldRdS(Flux, S, W);
    VectorXd psidRdSFD(nx + 1);
    psidRdSFD.setZero();
    psidRdSFD = xvec.transpose() * dRdS;
    std::cout<<"(psidRdSFD - psidRdS).norm()"<<std::endl;
    std::cout<<(psidRdSFD - psidRdS).norm()<<std::endl;
    
    // Evaluate dSdDesign
    MatrixXd dSdDesign(nx + 1, designVar.size());
    double d1 = designVar[0], d2 = designVar[1], d3 = designVar[2];
    double xh;
    for(int i = 0; i < nx + 1; i++)
    {
        if(i == 0 || i == nx)
        {
            dSdDesign(i, 0) = 0;
            dSdDesign(i, 1) = 0;
            dSdDesign(i, 2) = 0;
        }
        else
        {
            xh = fabs(x[i] - dx[i] / 2.0);
            dSdDesign(i, 0) = - pow(sin(PI * pow(xh, d2)), d3);
            dSdDesign(i, 1) = - d1 * d3 * PI * pow(xh, d2)
                              * cos(PI * pow(xh, d2)) * log(xh)
                              * pow(sin(PI * pow(xh, d2)), d3 - 1);
            dSdDesign(i, 2) = - d1 * log(sin(PI * pow(xh, d2)))
                              * pow(sin(PI * pow(xh, d2)), d3);
        }
    }

    VectorXd grad(designVar.size());
    grad = psidRdS.transpose() * dSdDesign;

    std::vector <double> gradient(designVar.size());
    for(int iDes = 0; iDes < designVar.size(); iDes++)
    {
       gradient[iDes] = grad(iDes);
    }
    std::cout<<"Gradient from Adjoint:"<<std::endl;
    std::cout<<std::setprecision(15)<<grad<<std::endl;
    return gradient;
}


// Calculates Jacobian
// Steger-Warming Flux Splitting
void StegerJac(std::vector <double> W,
               std::vector <double> &Ap_list,
               std::vector <double> &An_list,
               std::vector <double> &Flux)
{
    double eps = 0.1;
    double gam = 1.4;
    double M[3][3] = {{0}},
           Minv[3][3] = {{0}},
           N[3][3] = {{0}},
           Ninv[3][3] = {{0}},
           lambdaP[3][3],
           lambdaN[3][3];
    double lambdaa[3];
    
    
    double Ap[3][3], An[3][3], tempP[3][3], tempN[3][3], prefix[3][3], suffix[3][3];
    
    std::vector <double> rho(nx), u(nx), p(nx), c(nx);
    std::vector <double> Ap_list1(nx * 3 * 3, 0), An_list1(nx * 3 * 3, 0);

    double beta = gam - 1;

    for(int i = 0; i < nx; i++)
    {
        rho[i] = W[i * 3 + 0];
        u[i] = W[i * 3 + 1] / rho[i];
        p[i] = (gam-1) * (W[i * 3 + 2] - rho[i] * pow(u[i], 2) / 2);
        c[i] = sqrt( gam * p[i] / rho[i] );
    }


    for(int i = 0; i < nx; i++)
    {
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            Ap[row][col] = 0;
            An[row][col] = 0;
            tempP[row][col] = 0;
            tempN[row][col] = 0;
            prefix[row][col] = 0;
            suffix[row][col] = 0;
            lambdaP[row][col] = 0;
            lambdaN[row][col] = 0;
        }
    
        M[0][0] = 1.0;
        M[1][0] = -u[i] / rho[i];
        M[2][0] = 0.5 * u[i] * u[i] * beta;
        M[1][1] = 1.0 / rho[i];
        M[2][1] = -u[i] * beta;
        M[2][2] = beta;
        Minv[0][0] = 1.0;
        Minv[1][0] = u[i];
        Minv[2][0] = 0.5 * u[i] * u[i];
        Minv[1][1] = rho[i];
        Minv[2][1] = u[i] * rho[i];
        Minv[2][2] = 1.0 / beta;
        N[0][0] = 1.0;
        N[1][1] = rho[i] * c[i];
        N[2][1] = -rho[i] * c[i];
        N[0][2] = -1.0 / (c[i] * c[i]);
        N[1][2] = 1.0;
        N[2][2] = 1.0;
        Ninv[0][0] = 1.0;
        Ninv[0][1] = 1.0 / (2.0 * c[i] * c[i]);
        Ninv[0][2] = 1.0 / (2.0 * c[i] * c[i]);
        Ninv[1][1] = 1.0 / (2.0 * rho[i] * c[i]);
        Ninv[1][2] = -1.0 / (2.0 * rho[i] * c[i]);
        Ninv[2][1] = 0.5;
        Ninv[2][2] = 0.5;
        lambdaa[0] = u[i];
        lambdaa[1] = u[i] + c[i];
        lambdaa[2] = u[i] - c[i];
        
        for(int k = 0; k < 3; k++)
            if(lambdaa[k] > 0)
                lambdaP[k][k] = (lambdaa[k] + sqrt(pow(lambdaa[k], 2) + pow(eps, 2))) / 2.0;
            else
                lambdaN[k][k] = (lambdaa[k] - sqrt(pow(lambdaa[k], 2) + pow(eps, 2))) / 2.0;

        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
            for(int k = 0; k < 3; k++)
            {
                prefix[row][col]+= Minv[row][k] * Ninv[k][col];
                suffix[row][col]+= N[row][k] * M[k][col];
            }
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
            for(int k = 0; k < 3; k++)
            {
                tempP[row][col] += prefix[row][k] * lambdaP[k][col];
                tempN[row][col] += prefix[row][k] * lambdaN[k][col];
            }
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
            for(int k = 0; k < 3; k++)
            {
                Ap[row][col]+= tempP[row][k] * suffix[k][col];
                An[row][col]+= tempN[row][k] * suffix[k][col];
            }
        // could remove above loop and just use aplist and anlist
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            int vec_pos = (i * 3 * 3) + (row * 3) + col;
            Ap_list1[vec_pos] = Ap[row][col];
            An_list1[vec_pos] = An[row][col];
        }

    }

    for(int i = 1; i < nx; i++)
    {
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            int Ap_pos = ((i - 1) * 3 * 3) + (row * 3) + col;
            int An_pos = (i * 3 * 3) + (row * 3) + col;
            Flux[i * 3 + row] += Ap_list1[Ap_pos] * W[(i - 1) * 3 + col]
                                 + An_list1[An_pos] * W[i * 3 + col];
        }
    }
    
//  // Transpose the Jacobians
//  for(int i = 0; i < nx; i++)
//  for(int row = 0; row < 3; row++)
//  for(int col = 0; col < 3; col++)
//  {
//      // TRANSPOSED JACOBIANS
//      int vec_pos = (i * 3 * 3) + (row * 3) + col;
//      int vec_post = (i * 3 * 3) + (col * 3) + row;
//      Ap_list[vec_pos] = Ap_list1[vec_post];
//      An_list[vec_pos] = An_list1[vec_post];
//  }

}

void JacobianCenter(std::vector <double> &J,
                    double u, double c)
{
    J[0] = 0.0;
    J[1] = 1.0;
    J[2] = 0.0;
    J[3] = u * u * (gam - 3.0) / 2.0;
    J[4] = u * (3.0 - gam);
    J[5] = gam - 1.0;
    J[6] = ( pow(u, 3) * (gam - 1.0) * (gam - 2.0) - 2.0 * u * c * c ) / (2.0 * (gam - 1.0));
    J[7] = ( 2.0 * c * c + u * u * ( -2.0 * gam * gam + 5.0 * gam - 3.0 ) ) 
           / (2.0 * (gam - 1.0));
    J[8] = u * gam;
}

void ScalarJac(std::vector <double> W,
               std::vector <double> &Ap_list,
               std::vector <double> &An_list,
               std::vector <double> &Flux)
{
    std::vector <double> rho(nx), u(nx), e(nx);
    std::vector <double> T(nx), p(nx), c(nx), Mach(nx);
    WtoP(W, rho, u, e, p, c, T); 

    int vec_pos, k;
    double lamb;

    std::vector <double> J(9, 0);
    std::vector <double> dlambdadWp(3, 0);
    std::vector <double> dlambdaPdW(3, 0);
    std::vector <double> dlambdaNdW(3, 0);
    double dlambdadc, dcdr, dcdp;
    double dlambdadr, dlambdadu, dlambdadp;
    std::vector <double> dwpdw(9, 0);
    // A+
    for(int i = 0; i < nx - 1; i++)
    {
        // dF/dW
        JacobianCenter(J, u[i], c[i]);

        // lambda
        lamb = (u[i] + u[i + 1] + c[i] + c[i + 1]) / 2.0;
        // dlambdaP/dW
        dlambdadc = 0.5;
        dcdr = - p[i] * gam / (2.0 * c[i] * rho[i] * rho[i]);
        dcdp = gam / (2.0 * c[i] * rho[i]);

        dlambdadr = dlambdadc * dcdr;
        dlambdadu = 0.5;
        dlambdadp = dlambdadc * dcdp;

        dlambdadWp[0] = dlambdadr;
        dlambdadWp[1] = dlambdadu;
        dlambdadWp[2] = dlambdadp;
        dWpdW(dwpdw, W, i);
        dlambdaPdW[0] = 0;
        dlambdaPdW[1] = 0;
        dlambdaPdW[2] = 0;
        for(int row = 0; row < 1; row++)
        for(int col = 0; col < 3; col++)
        for(int k = 0; k < 3; k++)
            dlambdaPdW[row * 3 + col] += dlambdadWp[row * 3 + k] * dwpdw[k * 3 + col];

        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            vec_pos = (i * 9) + (row * 3) + col; // NOT Transposed
            k = row * 3 + col;
            Ap_list[vec_pos] = J[k] / 2.0 - dlambdaPdW[col] * Scalareps
                               * (W[(i + 1) * 3 + row] - W[i * 3 + row]) / 2.0;
            if(row == col)
            {
                Ap_list[vec_pos] += Scalareps * lamb / 2.0;
            }
        }
    }
    // dF/dW (Jacobian at the Outlet if using interpolated ghost cell)
//  JacobianCenter(J, u[nx-1], c[nx-1]);
//  for(int row = 0; row < 3; row++)
//  for(int col = 0; col < 3; col++)
//  {
//      vec_pos = ((nx - 1) * 9) + (row * 3) + col; // NOT Transposed
//      k = row * 3 + col;
//      Ap_list[vec_pos] = J[k];
//  }
    
    // A-
    for(int i = 1; i < nx; i++)
    {
        // dF/dW
        JacobianCenter(J, u[i], c[i]);

        // lambda
        lamb = (u[i] + u[i - 1] + c[i] + c[i - 1]) / 2.0;
        // dlambdaP/dW
        dlambdadc = 0.5;
        dcdr = - p[i] * gam / (2.0 * c[i] * rho[i] * rho[i]);
        dcdp = gam / (2.0 * c[i] * rho[i]);

        dlambdadr = dlambdadc * dcdr;
        dlambdadu = 0.5;
        dlambdadp = dlambdadc * dcdp;

        dlambdadWp[0] = dlambdadr;
        dlambdadWp[1] = dlambdadu;
        dlambdadWp[2] = dlambdadp;
        dWpdW(dwpdw, W, i);
        dlambdaNdW[0] = 0;
        dlambdaNdW[1] = 0;
        dlambdaNdW[2] = 0;
        for(int row = 0; row < 1; row++)
        for(int col = 0; col < 3; col++)
        for(int k = 0; k < 3; k++)
            dlambdaNdW[row * 3 + col] += dlambdadWp[row * 3 + k] * dwpdw[k * 3 + col];

        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            vec_pos = (i * 9) + (row * 3) + col; // NOT Transposed
            k = row * 3 + col;
            An_list[vec_pos] = J[k] / 2.0 - dlambdaNdW[col] * Scalareps
                               * (W[i * 3 + row] - W[(i - 1) * 3 + row]) / 2.0;
            if(row == col)
            {
                An_list[vec_pos] -= Scalareps * lamb / 2.0;
            }
        }
    }

    double avgu, avgc;
    int ki, kim;
    std::vector <double> F(3 * nx, 0);
    WtoF(W, F);
    for(int i = 1; i < nx; i++)
    {
        avgu = ( u[i - 1] + u[i] ) / 2.0;
        avgc = ( c[i - 1] + c[i] ) / 2.0;
        lamb = std::max( std::max( fabs(avgu), fabs(avgu + avgc) ),
                           fabs(avgu - avgc) );

        for(int k = 0; k < 3; k++)
        {
            ki = i * 3 + k;
            kim = (i - 1) * 3 + k;
            Flux[ki] = 0.5 * (F[kim] + F[ki]) - 0.5 * Scalareps * lamb * (W[ki] - W[kim]);
        }
    }

}



void evaldIcdW(std::vector <double> &dIcdW,
               std::vector <double> W,
               std::vector <double> dx)
{
    std::vector <double> ptarget(nx, 0);
    double dpdw[3], rho, u, p;
    ioTargetPressure(-1, ptarget);
    for(int i = 0; i < nx; i++)
    {
        rho = W[i * 3 + 0];
        u = W[i * 3 + 1] / rho;
        p = (gam - 1) * ( W[i * 3 + 2] - rho * u * u / 2.0 );

        dpdw[0] = (gam - 1) / 2.0 * u * u;
        dpdw[1] = - (gam - 1) * u;
        dpdw[2] = (gam - 1);

        dIcdW[i * 3 + 0] = (p / ptin - ptarget[i]) * dpdw[0] * dx[i] / ptin;
        dIcdW[i * 3 + 1] = (p / ptin - ptarget[i]) * dpdw[1] * dx[i] / ptin;
        dIcdW[i * 3 + 2] = (p / ptin - ptarget[i]) * dpdw[2] * dx[i] / ptin;
    }
}

void evaldQdW(std::vector <double> &dQdW,
                   std::vector <double> W,
                   std::vector <double> S)
{
    double dpdw[3], rho, u, dS;
    for(int i = 0; i < nx; i++)
    {
        rho = W[i * 3 + 0];
        u = W[i * 3 + 1] / rho;

        dpdw[0] = (gam - 1) / 2.0 * u * u;
        dpdw[1] = - (gam - 1) * u;
        dpdw[2] = (gam - 1);

        dS = S[i + 1] - S[i];

        dQdW[i * 3 + 0] = dpdw[0] * dS;
        dQdW[i * 3 + 1] = dpdw[1] * dS;
        dQdW[i * 3 + 2] = dpdw[2] * dS;
    }
}

void BCJac(std::vector <double> W,
           std::vector <double> dt,
           std::vector <double> dx,
           std::vector <double> ddtdW,
           std::vector <double> &dBidWi,
           std::vector <double> &dBidWd,
           std::vector <double> &dBodWd,
           std::vector <double> &dBodWo,
           std::vector <double> &B1,
           std::vector <double> &BN)
{
    std::vector <double> rho(nx), u(nx), e(nx), p(nx), c(nx), T(nx);
    std::vector <double> dbdwp(9, 0), dwpdw(9);

    for(int i = 0; i < 9; i++)
    {
        dBidWi[i] = 0;
        dBidWd[i] = 0;
        dBodWd[i] = 0;
        dBodWo[i] = 0;
    }

    WtoP(W, rho, u, e, p, c, T);

    // ************************
    // OUTLET JACOBIANS
    // ************************

    double i1, i2;
    double r1, r2, p1, p2, u1, u2, c1, c2, t1;
    i1 = nx - 1;
    i2 = nx - 2;
    r1 = rho[i1];
    r2 = rho[i2];
    p1 = p[i1];
    p2 = p[i2];
    u1 = u[i1];
    u2 = u[i2];
    c1 = c[i1];
    c2 = c[i2];
    t1 = T[i1];

    // Shorthand
    double gamr, fu, drho, dp, du, cr, uu;
    drho = r1 - r2;
    dp = p1 - p2;
    du = u1 - u2;
    cr = r1 * c1;
    uu = u1 * u1;

    // Speed of Sound
    double dc1dr1, dc2dr2, dc1dp1, dc2dp2;
    dc1dr1 = - p1 * gam / (2.0 * cr * r1);
    dc2dr2 = - p2 * gam / (2.0 * c2 * r2 * r2);
    dc1dp1 = gam / (2.0 * cr);
    dc2dp2 = gam / (2.0 * c2 * r2);

    double eig1, eig2, eig3;
    double deig1du1, deig1du2;
    double deig2dr1, deig2du1, deig2dp1, deig2dr2, deig2du2, deig2dp2;
    double deig3dr1, deig3du1, deig3dp1, deig3dr2, deig3du2, deig3dp2;
    // Eigenvalue
    eig1 = (u1 + u2) / 2.0;
    eig2 = eig1 + (c1 + c2) / 2.0;
    eig3 = eig1 - (c1 + c2) / 2.0;

    deig1du1 = 0.5;
    deig1du2 = 0.5;

    deig2dr1 = dc1dr1 / 2.0;
    deig2du1 = deig1du1;
    deig2dp1 = dc1dp1 / 2.0;
    deig2dr2 = dc2dr2 / 2.0;
    deig2du2 = deig1du2;
    deig2dp2 = dc2dp2 / 2.0;

    deig3dr1 = - dc1dr1 / 2.0;
    deig3du1 = deig1du1;
    deig3dp1 = - dc1dp1 / 2.0;
    deig3dr2 = - dc2dr2 / 2.0;
    deig3du2 = deig1du2;
    deig3dp2 = - dc2dp2 / 2.0;

    // Riemann invariants
    double R1, R2, R3;
    double dR1dr1, dR1du1, dR1dp1, dR1dr2, dR1du2, dR1dp2;
    double dR2dr1, dR2du1, dR2dp1, dR2dr2, dR2du2, dR2dp2;
    double dR3dr1, dR3du1, dR3dp1, dR3dr2, dR3du2, dR3dp2;
    R1 = - eig1 * (drho - dp / (c1 * c1));
    R2 = - eig2 * (dp + cr * du);
    R3 = - eig3 * (dp - cr * du);

    dR1dr1 = - eig1 * (1.0 + 2.0 * dp * dc1dr1 / pow(c1, 3) );
    dR1du1 = deig1du1 * (dp - c1 * c1 * drho) / (c1 * c1);
    dR1dp1 = eig1 * (c1 - 2 * dp * dc1dp1) / pow(c1, 3);
    dR1dr2 = eig1;
    dR1du2 = deig1du2 * (dp - c1 * c1 * drho) / (c1 * c1);
    dR1dp2 = - eig1 / (c1 * c1);

    dR2dr1 = - du * eig2 * (c1 + r1 * dc1dr1) - (dp + cr * du) * deig2dr1;
    dR2du1 = - cr * eig2 - (dp + cr * du) * deig2du1;
    dR2dp1 = - eig2 * (1.0 + du * r1 * dc1dp1) - (dp + cr * du) * deig2dp1;
    dR2dr2 = - (dp + cr * du) * deig2dr2;
    dR2du2 = cr * eig2 - (dp + cr * du) * deig2du2;
    dR2dp2 = eig2 - (dp + cr * du) * deig2dp2;

//  dR3dr1 = c1 * du * eig3 + du * eig3 * r1 * dc1dr1 + (-dp + cr * du) * deig3dr1; 
//  dR3du1 = - cr * eig3 + (-dp + cr * du) * deig3du1;
//  dR3dp1 = - eig3 + du * eig3 * r1 * dc1dp1 + (-dp + cr * du) * deig3dp1;
//  dR3dr2 = (-dp + cr * du) * deig3dr2;
//  dR3du2 = - cr * eig3 + (-dp + cr * du) * deig3du2;
//  dR3dp2 = eig3 + (-dp + cr * du) * deig3dp2;

//  std::cout<<"dr3dr1"<<dR3dr1<<std::endl;
//  std::cout<<"dr3du1"<<dR3du1<<std::endl;
//  std::cout<<"dr3dp1"<<dR3dp1<<std::endl;
//  std::cout<<"dr3dr2"<<dR3dr2<<std::endl;
//  std::cout<<"dr3du2"<<dR3du2<<std::endl;
//  std::cout<<"dr3dp2"<<dR3dp2<<std::endl;

    dR3dr1 = eig3 * du * (c1 + r1 * dc1dr1) - (dp - cr * du) * deig3dr1; 
    dR3du1 = cr * eig3 - (dp - cr * du) * deig3du1;
    dR3dp1 = - eig3 - dp * deig3dp1 + du * r1 * eig3 * dc1dp1;
    dR3dr2 = - (dp - cr * du) * deig3dr2;
    dR3du2 = - cr * eig3 - (dp - cr * du) * deig3du2;
    dR3dp2 = eig3 - (dp - cr * du) * deig3dp2;

//  std::cout<<"dr3dr1"<<dR3dr1<<std::endl;
//  std::cout<<"dr3du1"<<dR3du1<<std::endl;
//  std::cout<<"dr3dp1"<<dR3dp1<<std::endl;
//  std::cout<<"dr3dr2"<<dR3dr2<<std::endl;
//  std::cout<<"dr3du2"<<dR3du2<<std::endl;
//  std::cout<<"dr3dp2"<<dR3dp2<<std::endl;
    // dp1/dt
    double dp1dt;
    double dp1dtdr1, dp1dtdu1, dp1dtdp1;
    double dp1dtdr2, dp1dtdu2, dp1dtdp2;
    if(u1 < c1)
    {
        dp1dt = 0;
        dp1dtdr1 = 0;
        dp1dtdu1 = 0;
        dp1dtdp1 = 0;
        dp1dtdr2 = 0;
        dp1dtdu2 = 0;
        dp1dtdp2 = 0;
    }
    else
    {
        dp1dt = (R2 + R3) / 2.0;
        dp1dtdr1 = (dR2dr1 + dR3dr1) / 2.0;
        dp1dtdu1 = (dR2du1 + dR3du1) / 2.0;
        dp1dtdp1 = (dR2dp1 + dR3dp1) / 2.0;
        dp1dtdr2 = (dR2dr2 + dR3dr2) / 2.0;
        dp1dtdu2 = (dR2du2 + dR3du2) / 2.0;
        dp1dtdp2 = (dR2dp2 + dR3dp2) / 2.0;
    }

    // drho1/dt
    double dr1dt;
    double dr1dtdr1, dr1dtdu1, dr1dtdp1;
    double dr1dtdr2, dr1dtdu2, dr1dtdp2;
    dr1dt = R1 + dp1dt / (c1 * c1);

    dr1dtdr1 = dR1dr1 + dp1dtdr1 / (c1 * c1) - 2.0 * dp1dt * dc1dr1 / pow(c1, 3);
    dr1dtdu1 = dR1du1 + dp1dtdu1 / (c1 * c1);
    dr1dtdp1 = dR1dp1 + dp1dtdp1 / (c1 * c1) - 2.0 * dp1dt * dc1dp1 / pow(c1, 3);
    dr1dtdr2 = dR1dr2 + dp1dtdr2 / (c1 * c1);
    dr1dtdu2 = dR1du2 + dp1dtdu2 / (c1 * c1);
    dr1dtdp2 = dR1dp2 + dp1dtdp2 / (c1 * c1);

    // du1/dt
    double du1dt;
    double du1dtdr1, du1dtdu1, du1dtdp1;
    double du1dtdr2, du1dtdu2, du1dtdp2;
    du1dt = (R2 - dp1dt) / (cr);

    du1dtdr1 = ( (dp1dt - R2) * r1 * dc1dr1
               + c1 * (dp1dt - R2 - r1 * dp1dtdr1 + r1 * dR2dr1) )
               / (cr * cr);
    du1dtdu1 = (dR2du1 - dp1dtdu1) / cr;
    du1dtdp1 = ( (dp1dt - R2) * dc1dp1 + c1 * (dR2dp1 - dp1dtdp1) ) / (cr * c1);
    du1dtdr2 = (dR2dr2 - dp1dtdr2) / cr;
    du1dtdu2 = (dR2du2 - dp1dtdu2) / cr;
    du1dtdp2 = (dR2dp2 - dp1dtdp2) / cr;

    // d(ru)1/dt
    double dru1dt;
    dru1dt = r1 * du1dt + u1 * dr1dt;
    double dru1dtdr1, dru1dtdu1, dru1dtdp1;
    double dru1dtdr2, dru1dtdu2, dru1dtdp2;
    dru1dtdr1 = du1dt + u1 * dr1dtdr1 + r1 * du1dtdr1;
    dru1dtdu1 = dr1dt + u1 * dr1dtdu1 + r1 * du1dtdu1;
    dru1dtdp1 = u1 * dr1dtdp1 + r1 * du1dtdp1;
    dru1dtdr2 = u1 * dr1dtdr2 + r1 * du1dtdr2;
    dru1dtdu2 = u1 * dr1dtdu2 + r1 * du1dtdu2;
    dru1dtdp2 = u1 * dr1dtdp2 + r1 * du1dtdp2;

    // de1/dt
    double de1dt;
    de1dt = dp1dt * Cv / R + u1 * r1 * du1dt + uu * dr1dt / 2.0;
    double de1dtdr1, de1dtdu1, de1dtdp1;
    double de1dtdr2, de1dtdu2, de1dtdp2;

//  de1dtdr1 = du1dt * u1 + dp1dtdr1 * Cv / R + uu * dr1dtdr1 / 2.0 + r1 * u1 * du1dtdr1;
//  de1dtdu1 = dr1dt * u1 + du1dt * r1 + dp1dtdu1 * Cv / R
//            + uu * dr1dtdu1 / 2.0 + r1 * u1 * du1dtdu1;
//  de1dtdp1 = dp1dtdp1 * Cv / R + uu * dr1dtdp1 / 2.0 + r1 * u1 * du1dtdp1;
//  de1dtdr2 = dp1dtdr2 * Cv / R + u1 + r1 * u1 * du1dtdr2 + uu * dr1dtdr2 / 2.0;
//  de1dtdu2 = dp1dtdu2 * Cv / R + u1 + r1 * u1 * du1dtdu2 + uu * dr1dtdu2 / 2.0;
//  de1dtdp2 = dp1dtdp2 * Cv / R + u1 + r1 * u1 * du1dtdp2 + uu * dr1dtdp2 / 2.0;

    de1dtdr1 = dp1dtdr1 * Cv / R + uu * dr1dtdr1 / 2.0 + r1 * u1 * du1dtdr1 
               + du1dt * u1;
    de1dtdu1 = dp1dtdu1 * Cv / R + uu * dr1dtdu1 / 2.0 + r1 * u1 * du1dtdu1 
               + du1dt * r1 + dr1dt * u1;
    de1dtdp1 = dp1dtdp1 / (gam - 1) + uu * dr1dtdp1 / 2.0 + r1 * u1 * du1dtdp1;
    de1dtdr2 = dp1dtdr2 / (gam - 1) + uu * dr1dtdr2 / 2.0 + r1 * u1 * du1dtdr2;
    de1dtdu2 = dp1dtdu2 / (gam - 1) + uu * dr1dtdu2 / 2.0 + r1 * u1 * du1dtdu2;
    de1dtdp2 = dp1dtdp2 / (gam - 1) + uu * dr1dtdp2 / 2.0 + r1 * u1 * du1dtdp2;

    // BN
    BN[0] = dr1dt;
    BN[1] = dru1dt;
    BN[2] = de1dt;

    dbdwp[0] = dr1dtdr1;
    dbdwp[1] = dr1dtdu1;
    dbdwp[2] = dr1dtdp1;
    dbdwp[3] = dru1dtdr1;
    dbdwp[4] = dru1dtdu1;
    dbdwp[5] = dru1dtdp1;
    dbdwp[6] = de1dtdr1;
    dbdwp[7] = de1dtdu1;
    dbdwp[8] = de1dtdp1;

    std::cout.precision(17);
    // Get Transformation Matrix
    dWpdW(dwpdw, W, nx - 1);

    // Note that the matrix is NOT TRANSPOSED
    for(int row = 0; row < 3; row++)
    for(int col = 0; col < 3; col++)
    for(int k = 0; k < 3; k++)
        dBodWo[row * 3 + col] += dbdwp[row * 3 + k] * dwpdw[k * 3 + col];
    
    dbdwp[0] = dr1dtdr2;
    dbdwp[1] = dr1dtdu2;
    dbdwp[2] = dr1dtdp2;
    dbdwp[3] = dru1dtdr2;
    dbdwp[4] = dru1dtdu2;
    dbdwp[5] = dru1dtdp2;
    dbdwp[6] = de1dtdr2;
    dbdwp[7] = de1dtdu2;
    dbdwp[8] = de1dtdp2;

    // Get Transformation Matrix
    dWpdW(dwpdw, W, nx - 2);

    // Note that the matrix is NOT TRANSPOSED
    for(int row = 0; row < 3; row++)
    for(int col = 0; col < 3; col++)
    for(int k = 0; k < 3; k++)
        dBodWd[row * 3 + col] += dbdwp[row * 3 + k] * dwpdw[k * 3 + col];

    // *********************
    // INLET JACOBIANS
    // *********************
    // Subsonic Inlet
    if(u[0] < c[0])
    {
        i1 = 0;
        i2 = 1;
        r1 = rho[i1];
        r2 = rho[i2];
        p1 = p[i1];
        p2 = p[i2];
        u1 = u[i1];
        u2 = u[i2];
        c1 = c[i1];
        c2 = c[i2];
        t1 = T[i1];

        // Shorthand
        drho = r2 - r1;
        dp = p2 - p1;
        du = u2 - u1;
        cr = r1 * c1;
        uu = u1 * u1;
        gamr = (gam - 1) / (gam + 1);
        fu = 1 - gamr * u1 * u1 / a2;

        // Speed of Sound
        dc1dr1 = - p1 * gam / (2.0 * cr * r1);
        dc2dr2 = - p2 * gam / (2.0 * c2 * r2 * r2);
        dc1dp1 = gam / (2.0 * cr);
        dc2dp2 = gam / (2.0 * c2 * r2);

        // Eigenvalue
        eig1 = (u1 + u2) / 2.0;
        eig3 = eig1 - (c1 + c2) / 2.0;

        deig1du1 = 0.5;
        deig1du2 = 0.5;

        deig3dr1 = - dc1dr1 / 2.0;
        deig3du1 = deig1du1;
        deig3dp1 = - dc1dp1 / 2.0;
        deig3dr2 = - dc2dr2 / 2.0;
        deig3du2 = deig1du2;
        deig3dp2 = - dc2dp2 / 2.0;

        // Riemann Invariants
        R3 = - eig3 * (dp - cr * du);

        dR3dr1 = eig3 * (c1 * du + du * r1 * dc1dr1) - (dp - cr * du) * deig3dr1; 
        dR3du1 = - cr * eig3 - (dp - cr * du) * deig3du1;
        dR3dp1 = eig3 * (du * r1 * dc1dp1 + 1.0) - (dp - cr * du) * deig3dp1;
        dR3dr2 = -(dp - cr * du) * deig3dr2;
        dR3du2 = cr * eig3 - (dp - cr * du) * deig3du2;
        dR3dp2 = - eig3 - (dp - cr * du) * deig3dp2;


        // dp1
        double dp1du1, dp1du1du1;
//      dp1du1 = -2 * ptin * pow(fu, 1 / (gam - 1)) * gamr * u1 / a2 * (gam / (gam - 1));
//      dp1du1du1 = 2 * gamr * ptin * pow(fu, 1 / (gam - 1)) * gam
//                  * (a2 - a2 * gam + gamr * uu * (gam + 1))
//                  / (a2 * (a2 - gamr * uu) * pow(gam - 1, 2));
        // Same Values
        dp1du1 = -2.0 * gamr * ptin * u1 * pow(fu, 1.0 / (gam - 1.0)) * gam
                 / (a2 * (gam - 1.0));
        dp1du1du1 = 2 * gamr * ptin * pow(fu, gam/(gam - 1.0)) * gam
                    * (a2 - a2 * gam + gamr * uu * (gam + 1))
                    / pow((a2 - gamr * uu) * (gam - 1.0), 2);

        // du1
        du1dt = R3 / (dp1du1 - cr);
        du1dtdr1 = dR3dr1 / (dp1du1 - cr)
                   + R3 * (c1 + r1 * dc1dr1) / pow((dp1du1 - cr), 2);
        du1dtdu1 = dR3du1 / (dp1du1 - cr)
                   - R3 * dp1du1du1 / pow((dp1du1 - cr), 2);
        du1dtdp1 = (R3 * r1 * dc1dp1) / pow((dp1du1 - cr), 2)
                   + dR3dp1 / (dp1du1 - cr);
        du1dtdr2 = dR3dr2 / (dp1du1 - cr);
        du1dtdu2 = dR3du2 / (dp1du1 - cr);
        du1dtdp2 = dR3dp2 / (dp1du1 - cr);

        // dp1/dt
        dp1dt  = dp1du1 * du1dt;
        dp1dtdr1 = dp1du1 * du1dtdr1;
        dp1dtdu1 = du1dt * dp1du1du1 + dp1du1 * du1dtdu1;
        dp1dtdp1 = dp1du1 * du1dtdp1;
        dp1dtdr2 = dp1du1 * du1dtdr2;
        dp1dtdu2 = dp1du1 * du1dtdu2;
        dp1dtdp2 = dp1du1 * du1dtdp2;

        // dt1
        double dt1dp1, dt1dp1dp1;
        dt1dp1 = Ttin / ptin * (gam - 1.0) / gam * pow(p1 / ptin, - 1.0 / gam);
        dt1dp1dp1 = - Ttin * (gam - 1.0) / pow((gam * ptin), 2)
                    * pow(p1 / ptin, - (1.0 + gam) / gam);

        // dr1/dt
        double dr1dp1, dr1dp1dp1;
        dr1dp1 = 1.0 / (R * t1) - p1 * dt1dp1 / (R * t1 * t1);
        dr1dp1dp1 = 2.0 * p1 * dt1dp1 * dt1dp1 / (R * t1 * t1 * t1)
                    - 2.0 * dt1dp1 / (R * t1 * t1)
                    - p1 * dt1dp1dp1 / (R * t1 * t1);
        dr1dt = dr1dp1 * dp1dt;

        dr1dtdr1 = dr1dp1 * dp1dtdr1;
        dr1dtdu1 = dr1dp1 * dp1dtdu1;
        dr1dtdp1 = dr1dp1 * dp1dtdp1 + dp1dt * dr1dp1dp1;
        dr1dtdr2 = dr1dp1 * dp1dtdr2;
        dr1dtdu2 = dr1dp1 * dp1dtdu2;
        dr1dtdp2 = dr1dp1 * dp1dtdp2;

        // dru1/dt
        dru1dt = r1 * du1dt + u1 * dr1dt;

        dru1dtdr1 = du1dt + u1 * dr1dtdr1 + r1 * du1dtdr1;
        dru1dtdu1 = dr1dt + u1 * dr1dtdu1 + r1 * du1dtdu1;
        dru1dtdp1 = u1 * dr1dtdp1 + r1 * du1dtdp1;
        dru1dtdr2 = u1 * dr1dtdr2 + r1 * du1dtdr2;
        dru1dtdu2 = u1 * dr1dtdu2 + r1 * du1dtdu2;
        dru1dtdp2 = u1 * dr1dtdp2 + r1 * du1dtdp2;

        // de1/dt
        de1dt = dp1dt * Cv / R + r1 * u1 * du1dt + uu * dr1dt / 2.0;
        de1dtdr1 = dp1dtdr1 * Cv / R + uu * dr1dtdr1 / 2.0 + r1 * u1 * du1dtdr1 
                   + du1dt * u1;
        de1dtdu1 = dp1dtdu1 * Cv / R + uu * dr1dtdu1 / 2.0 + r1 * u1 * du1dtdu1 
                   + du1dt * r1 + dr1dt * u1;
        de1dtdp1 = dp1dtdp1 / (gam - 1) + uu * dr1dtdp1 / 2.0 + r1 * u1 * du1dtdp1;
        de1dtdr2 = dp1dtdr2 / (gam - 1) + uu * dr1dtdr2 / 2.0 + r1 * u1 * du1dtdr2;
        de1dtdu2 = dp1dtdu2 / (gam - 1) + uu * dr1dtdu2 / 2.0 + r1 * u1 * du1dtdu2;
        de1dtdp2 = dp1dtdp2 / (gam - 1) + uu * dr1dtdp2 / 2.0 + r1 * u1 * du1dtdp2;

        // Same values
        //de1dtdr1 = Cv * dp1dt * dt1dp1 + du1dt * u1 + Cv * dt1dp1 * r1 * dp1dtdr1
        //           + (Cv * t1 + uu / 2) * dr1dtdr1 + u1 * r1 * du1dtdr1;
        //de1dtdu1 = dr1dt * u1 * du1dt * r1 + Cv * dt1dp1 * r1 * dp1dtdu1
        //           + (Cv * t1 + uu / 2) * dr1dtdu1 + u1 * r1 * du1dtdu1;
        //de1dtdp1 = Cv * dt1dp1 * r1 * dp1dtdp1 + (Cv * t1 + uu / 2) * dr1dtdp1
        //           + Cv * dp1dt * r1 * dt1dp1dp1 + u1 * r1 * du1dtdp1
        //           + Cv * dr1dt * dt1dp1;
        //de1dtdr2 = Cv * dt1dp1 * r1 * dp1dtdr2 + (Cv * t1 + uu / 2) * dr1dtdr2
        //           + u1 * r1 * du1dtdr2;
        //de1dtdu2 = Cv * dt1dp1 * r1 * dp1dtdu2 + (Cv * t1 + uu / 2) * dr1dtdu2
        //           + u1 * r1 * du1dtdu2;
        //de1dtdp2 = Cv * dt1dp1 * r1 * dp1dtdp2 + (Cv * t1 + uu / 2) * dr1dtdp2
        //           + u1 * r1 * du1dtdp2;

        // B1
        B1[0] = dr1dt;
        B1[1] = dru1dt;
        B1[2] = de1dt;

        dbdwp[0] = dr1dtdr1;
        dbdwp[1] = dr1dtdu1;
        dbdwp[2] = dr1dtdp1;
        dbdwp[3] = dru1dtdr1;
        dbdwp[4] = dru1dtdu1;
        dbdwp[5] = dru1dtdp1;
        dbdwp[6] = de1dtdr1;
        dbdwp[7] = de1dtdu1;
        dbdwp[8] = de1dtdp1;

        // Get Transformation Matrix
        dWpdW(dwpdw, W, 0);

        // Note that the matrix is Transposed
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        for(int k = 0; k < 3; k++)
            dBidWi[col * 3 + row] += dbdwp[row * 3 + k] * dwpdw[k * 3 + col];
        
        dbdwp[0] = dr1dtdr2;
        dbdwp[1] = dr1dtdu2;
        dbdwp[2] = dr1dtdp2;
        dbdwp[3] = dru1dtdr2;
        dbdwp[4] = dru1dtdu2;
        dbdwp[5] = dru1dtdp2;
        dbdwp[6] = de1dtdr2;
        dbdwp[7] = de1dtdu2;
        dbdwp[8] = de1dtdp2;

        // Get Transformation Matrix
        dWpdW(dwpdw, W, 1);

        // Note that the matrix is Transposed
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        for(int k = 0; k < 3; k++)
            dBidWd[col * 3 + row] += dbdwp[row * 3 + k] * dwpdw[k * 3 + col];
    }
    // Supersonic Inlet
    else
    {
        B1[0] = 0;
        B1[1] = 0;
        B1[2] = 0;
        for(int i = 0; i < 9; i++)
        {
            dBidWi[i] = 0;
            dBidWd[i] = 0;
            if(i % 4 == 0)
                dBidWi[i] = 1;
        }
    }
}

SparseMatrix<double> buildAMatrix(std::vector <double> Ap,
                                  std::vector <double> An,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min)
{
    SparseMatrix<double> matA(3 * nx, 3 * nx);
    int Ri, Wi;
    int k, rowi, coli;
    double val;
    // Input 4 lines where BC Jacobians occur
    // psi(1), psi(2), psi(n-1), psi(n)
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
            k = row * 3 + col;
            // d(inlet)/d(inlet)
            // R0, W0
            Ri = 0;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli= Wi * 3 + col;

            val = - dBidWi[k];
            matA.insert(rowi, coli) = val;

            // d(inlet)/d(domain)
            // R0, W1
            Ri = 0;
            Wi = 1;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBidWd[k];
            matA.insert(rowi, coli) = val;

            // d(outlet)/d(outlet)
            // R = nx - 1, W = nx - 1
            Ri = nx - 1;
            Wi = nx - 1;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBodWo[k];
            matA.insert(rowi, coli) = val;

            // d(outlet)/d(domain)
            // R = nx - 1, W = nx - 2
            Ri = nx - 1;
            Wi = nx - 2;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBodWd[k];
            matA.insert(rowi, coli) = val;
        }
    }
    for(int Ri = 1; Ri < nx - 1; Ri++)
    {
        Wi = Ri - 1;
        if(Wi >= 0)
        {
            for(int row = 0; row < 3; row++)
            for(int col = 0; col < 3; col++)
            {
                k = row * 3 + col;
                rowi = Ri * 3 + row;
                coli = Wi * 3 + col;

                val = - Ap[Wi * 9 + k] * S[Ri];
                matA.insert(rowi, coli) = val;
            }
        }

        Wi = Ri;
        if(Wi >= 0 && Wi <= nx - 1)
        {
            for(int row = 0; row < 3; row++)
            for(int col = 0; col < 3; col++)
            {
                k = row * 3 + col;
                rowi = Ri * 3 + row;
                coli = Wi * 3 + col;

                val = Ap[Wi * 9 + k] * S[Ri + 1];
                val -= An[Wi * 9 + k] * S[Ri];
                if(row == 1) // Remember it is NOT TRANSPOSED dQdW
                {
                    val -= dQdW[Wi * 3 + col];
                }

                matA.insert(rowi, coli) = val;
            }
        }

        Wi = Ri + 1;
        if(Wi <= nx - 1)
        {
            for(int row = 0; row < 3; row++)
            for(int col = 0; col < 3; col++)
            {
                k = row * 3 + col;
                rowi = Ri * 3 + row;
                coli = Wi * 3 + col;

                val = An[Wi * 9 + k] * S[Ri + 1];

                matA.insert(rowi, coli) = val;
            }
        }
    }
    if(Min > 1.0)
    {
        // Supersonic Inlet, don't solve for psi(0)
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            // R1, W0
            Ri = 1;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;
    
            matA.coeffRef(rowi, coli) = 0;
        }
    }
    return matA;
}

VectorXd buildbMatrix(std::vector <double> dIcdW)
{
    VectorXd matb(3 * nx);

    for(int i = 0; i < nx; i++)
    for(int k = 0; k < 3; k++)
        matb(i * 3 + k) = -dIcdW[i * 3 + k];
    
    return matb;
}

MatrixXd evaldRdS(std::vector <double> Flux, std::vector <double> S,
                  std::vector <double> W)
{
    MatrixXd dRdS(3 * nx, nx + 1);
    std::vector <double> Resi0(3 * nx, 0), Resi1(3 * nx, 0), Resi2(3 * nx, 0);
    std::vector <double> Sd(nx + 1, 0);
    std::vector <double> Q(3 * nx, 0);
    double h = 0.000001;
    double pert;
    int ki, kip;
    dRdS.setZero();
    for(int Ri = 1; Ri < nx - 1; Ri++)
    {
        for(int Si = 0; Si < nx + 1; Si++)
        {
            for(int m = 0; m < nx + 1; m++)
                Sd[m] = S[m];

            pert = S[Si] * h;
            Sd[Si] = S[Si] + pert;
            
            WtoQ(W, Q, Sd);
            
            for(int k = 0; k < 3; k++)
            {
                ki = Ri * 3 + k;
                kip = (Ri + 1) * 3 + k;
                Resi1[ki] = Flux[kip] * Sd[Ri + 1] - Flux[ki] * Sd[Ri] - Q[ki];
            }
            
            for(int m = 0; m < nx + 1; m++)
                Sd[m] = S[m];

            Sd[Si] = S[Si] - pert;

            WtoQ(W, Q, Sd);

            for(int k = 0; k < 3; k++)
            {
                ki = Ri * 3 + k;
                kip = (Ri + 1) * 3 + k;
                Resi2[ki] = Flux[kip] * Sd[Ri + 1] - Flux[ki] * Sd[Ri] - Q[ki];
                dRdS(ki, Si) = (Resi1[ki] - Resi2[ki]) / (2 * pert);
            }
        }
    }
    
    return dRdS;
}

void evalddtdW(std::vector <double> &ddtdW,
                 std::vector <double> rho,
                 std::vector <double> u,
                 std::vector <double> p,
                 std::vector <double> c,
                 std::vector <double> dx,
                 std::vector <double> S)
{
    double dStepdr, dStepdru, dStepde;
    for(int i = 0; i < nx; i++)
    {
        dStepdr  = ( 2 * p[i] * gam 
                   + u[i] * rho[i] * ( u[i] * (1 - gam) * gam + 4 * c[i] ) )
                   / ( 4 * pow(u[i] + c[i], 2) * c[i] * rho[i] * rho[i] );
        dStepdru = ( u[i] * gam * (gam - 1) - 2 * c[i] )
                   / ( 2 * pow(u[i] + c[i], 2) * c[i] * rho[i] );
        dStepde  = (1 - gam) * c[i]
                   / ( 2 * p[i] * pow(u[i] + c[i], 2) );

        ddtdW[i * 3 + 0] = CFL * dx[i] * dStepdr;
        ddtdW[i * 3 + 1] = CFL * dx[i] * dStepdru;
        ddtdW[i * 3 + 2] = CFL * dx[i] * dStepde;
    }
}

SparseMatrix<double> buildAFD(std::vector <double> W,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min)
{
    SparseMatrix<double> matA(3 * nx, 3 * nx);
    int Ri, Wi;
    int k, rowi, coli;
    double val;
    // Input 4 lines where BC Jacobians occur
    // psi(1), psi(2), psi(n-1), psi(n)
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
            k = row * 3 + col;
            // dw0, psi0
            Ri = 0;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBidWi[k];
            matA.insert(rowi, coli) = val;

            // dw1, psi0
            Ri = 0;
            Wi = 1;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBidWd[k];
            matA.insert(rowi, coli) = val;

//          // dw(nx-1), psi(nx-1)
//          Ri = nx - 1;
//          Wi = nx - 1;
//          rowi = Ri * 3 + row;
//          coli = Wi * 3 + col;

//          val = - dBodWo[k];
//          matA.insert(rowi, coli) = val;

//          // dw(nx-2), psi(nx-1)
//          Ri = nx - 2;
//          Wi = nx - 1;
//          rowi = Ri * 3 + row;
//          coli = Wi * 3 + col;

//          val = - dBodWd[k];
//          matA.insert(rowi, coli) = val;
        }
    }
    std::vector <double> Wd(3 * nx, 0), F(3 * nx, 0), Q(3 * nx, 0); 
    std::vector <double> Flux(3 * (nx + 1), 0);
    std::vector <double> Resi0(3 * nx, 0), Resi1(3 * nx, 0);
    std::vector <double> dRdW(9, 0);
    WtoF(W, F);
    WtoQ(W, Q, S);
    getFlux(Flux, W, F);
    int ki, kip;
    double pert;

    for(int k = 0; k < 3; k++)
    {
        for(int i = 1; i < nx - 1; i++)
        {
            ki = i * 3 + k;
            kip = (i + 1) * 3 + k;
            Resi0[ki] = Flux[kip] * S[i + 1] - Flux[ki] * S[i] - Q[ki];
        }
        Resi0[0 * 3 + k] = 0;
        Resi0[(nx - 1) * 3 + k] = 0;
    }
    for(int i = 0; i < 3 * nx; i++)
        Wd[i] = W[i];
    outletBC(Wd, 1, 1, Resi0);

    for(int Ri = 1; Ri < nx; Ri++)
    {
        for(int Wi = 0; Wi < nx; Wi++)
        {
            double h = 0.00001;
            // Outlet
            if (Ri == nx - 1)
            {
                for(int statei = 0; statei < 3; statei++)
                {
                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];
                    
                    pert = W[Wi * 3 + statei] * h;
                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] + pert;
                    outletBC(Wd, 1, 1, Resi1);
                    
                    for(int resii = 0; resii < 3; resii++)
                    {
                        ki = Ri * 3 + resii;
                        dRdW[resii * 3 + statei] = (Resi1[ki] - Resi0[ki]) / pert;
                    }
                }
                for(int row = 0; row < 3; row++)
                for(int col = 0; col < 3; col++)
                {
                    rowi = Ri * 3 + row;
                    coli = Wi * 3 + col;
                    matA.insert(rowi, coli) = dRdW[row * 3 + col]; // Transposed
                }
            }
            // Domain
            else
            {
                for(int statei = 0; statei < 3; statei++)
                {
                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];

                    pert = W[Wi * 3 + statei] * h;
                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] + pert;

                    WtoF(Wd, F);
                    WtoQ(Wd, Q, S);
                    getFlux(Flux, Wd, F);
                    
                    for(int resii = 0; resii < 3; resii++)
                    {
                        ki = Ri * 3 + resii;
                        kip = (Ri + 1) * 3 + resii;
                        Resi1[ki] = Flux[kip] * S[Ri + 1] - Flux[ki] * S[Ri] - Q[ki];
                        dRdW[resii * 3 + statei] = (Resi1[ki] - Resi0[ki]) / pert;
                    }
                }
                for(int row = 0; row < 3; row++)
                for(int col = 0; col < 3; col++)
                {
                    rowi = Ri * 3 + row;
                    coli = Wi * 3 + col;
                    matA.insert(rowi, coli) = dRdW[row * 3 + col]; // Transposed
                }
            }

        }
        
    }
    if(Min > 1.0)
    {
        // Supersonic Inlet, don't solve for psi(0)
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            // dw(0), psi(1)
            Ri = 1;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;
    
            matA.coeffRef(rowi, coli) = 0;
        }
    }
    return matA;
}

SparseMatrix<double> buildAFD2(std::vector <double> W,
                                  std::vector <double> dBidWi,
                                  std::vector <double> dBidWd,
                                  std::vector <double> dBodWd,
                                  std::vector <double> dBodWo,
                                  std::vector <double> dQdW,
                                  std::vector <double> dx,
                                  std::vector <double> dt,
                                  std::vector <double> S,
                                  double Min)
{
    SparseMatrix<double> matA(3 * nx, 3 * nx);
    int Ri, Wi;
    int k, rowi, coli;
    double val;
    // Input 4 lines where BC Jacobians occur
    // psi(1), psi(2), psi(n-1), psi(n)
    for(int row = 0; row < 3; row++)
    {
        for(int col = 0; col < 3; col++)
        {
            k = row * 3 + col;
            // dw0, psi0
            Ri = 0;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBidWi[k];
            matA.insert(rowi, coli) = val;

            // dw1, psi0
            Ri = 0;
            Wi = 1;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;

            val = - dBidWd[k];
            matA.insert(rowi, coli) = val;

//          // dw(nx-1), psi(nx-1)
//          Ri = nx - 1;
//          Wi = nx - 1;
//          rowi = Ri * 3 + row;
//          coli = Wi * 3 + col;

//          val = - dBodWo[k];
//          matA.insert(rowi, coli) = val;

//          // dw(nx-2), psi(nx-1)
//          Ri = nx - 2;
//          Wi = nx - 1;
//          rowi = Ri * 3 + row;
//          coli = Wi * 3 + col;

//          val = - dBodWd[k];
//          matA.insert(rowi, coli) = val;
        }
    }
    std::vector <double> Wd(3 * nx, 0), F(3 * nx, 0), Q(3 * nx, 0); 
    std::vector <double> Flux(3 * (nx + 1), 0);
    std::vector <double> Resi0(3 * nx, 0), Resi1(3 * nx, 0), Resi2(3 * nx, 0);
    std::vector <double> dRdW(9, 0);
    WtoF(W, F);
    WtoQ(W, Q, S);
    getFlux(Flux, W, F);
    int ki, kip;
    double pert;

    for(int k = 0; k < 3; k++)
    {
        for(int i = 1; i < nx - 1; i++)
        {
            ki = i * 3 + k;
            kip = (i + 1) * 3 + k;
            Resi0[ki] = Flux[kip] * S[i + 1] - Flux[ki] * S[i] - Q[ki];
        }
        Resi0[0 * 3 + k] = 0;
        Resi0[(nx - 1) * 3 + k] = 0;
    }
    for(int i = 0; i < 3 * nx; i++)
        Wd[i] = W[i];
    outletBC(Wd, 1, 1, Resi0);

    for(int Ri = 1; Ri < nx; Ri++)
    {
        for(int Wi = 0; Wi < nx; Wi++)
        {
            double h = 0.00001;
            // Outlet
            if (Ri == nx - 1)
            {
                for(int statei = 0; statei < 3; statei++)
                {
                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];
                    
                    pert = W[Wi * 3 + statei] * h;
                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] + pert;
                    outletBC(Wd, 1, 1, Resi1);
                    
                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];
                    
                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] - pert;
                    outletBC(Wd, 1, 1, Resi2);
                    
                    for(int resii = 0; resii < 3; resii++)
                    {
                        ki = Ri * 3 + resii;
                        dRdW[resii * 3 + statei] = (Resi1[ki] - Resi2[ki]) / (2 * pert);
                    }
                }
                for(int row = 0; row < 3; row++)
                for(int col = 0; col < 3; col++)
                {
                    rowi = Ri * 3 + row;
                    coli = Wi * 3 + col;
                    matA.insert(rowi, coli) = dRdW[row * 3 + col]; // Transposed
                }
            }
            // Domain
            else
            {
                for(int statei = 0; statei < 3; statei++)
                {
                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];

                    pert = W[Wi * 3 + statei] * h;
                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] + pert;

                    WtoF(Wd, F);
                    WtoQ(Wd, Q, S);
                    getFlux(Flux, Wd, F);
                    
                    for(int resii = 0; resii < 3; resii++)
                    {
                        ki = Ri * 3 + resii;
                        kip = (Ri + 1) * 3 + resii;
                        Resi1[ki] = Flux[kip] * S[Ri + 1] - Flux[ki] * S[Ri] - Q[ki];
                    }

                    for(int i = 0; i < 3 * nx; i++)
                        Wd[i] = W[i];

                    Wd[Wi * 3 + statei] = W[Wi * 3 + statei] - pert;

                    WtoF(Wd, F);
                    WtoQ(Wd, Q, S);
                    getFlux(Flux, Wd, F);
                    
                    for(int resii = 0; resii < 3; resii++)
                    {
                        ki = Ri * 3 + resii;
                        kip = (Ri + 1) * 3 + resii;
                        Resi2[ki] = Flux[kip] * S[Ri + 1] - Flux[ki] * S[Ri] - Q[ki];
                        dRdW[resii * 3 + statei] = (Resi1[ki] - Resi2[ki]) / (2 * pert);
                    }
                }
                for(int row = 0; row < 3; row++)
                for(int col = 0; col < 3; col++)
                {
                    rowi = Ri * 3 + row;
                    coli = Wi * 3 + col;
                    matA.insert(rowi, coli) = dRdW[row * 3 + col]; // Transposed
                }
            }

        }
        
    }
    if(Min > 1.0)
    {
        // Supersonic Inlet, don't solve for psi(0)
        for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++)
        {
            // dw(0), psi(1)
            Ri = 1;
            Wi = 0;
            rowi = Ri * 3 + row;
            coli = Wi * 3 + col;
    
            matA.coeffRef(rowi, coli) = 0;
        }
    }
    return matA;
}
