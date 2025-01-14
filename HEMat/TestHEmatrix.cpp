//!
//! @file       TestHEmatrix.cpp, cpp file
//! @brief      defining functions for testing homomorphic matrix computation
//!
//! @author     Miran Kim
//! @date       Dec. 1, 2017
//! @copyright  GNU Pub License
//!

#include <cmath>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <sys/time.h>
#include <chrono>

#include <NTL/RR.h>
#include <NTL/xdouble.h>
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>
#include "NTL/RR.h"
#include "NTL/vec_RR.h"
#include "NTL/mat_RR.h"
#include <NTL/BasicThreadPool.h>
#include <NTL/mat_ZZ.h>


#include "../src/Context.h"
#include "../src/Scheme.h"
#include "../src/SecretKey.h"
#include "../src/TimeUtils.h"

#include "matrix.h"
#include "HEmatrix.h"
#include "TestHEmatrix.h"

using namespace std;
using namespace chrono;


void TestHEmatrix::testHEAAN(long logN, long logQ) {

    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    
    long h = 64;
    
    long ntrial = 10;
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    

    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    
    long nslots = (1<<(logN-1));
    
    double* msg1 = new double[nslots];
    double* msg2 = new double[nslots];
    
    complex<double>* cmsg1 = new complex<double>[nslots];
    complex<double>* cmsg2 = new complex<double>[nslots];
    
    double* dmsg1 = new double[nslots];
    double* dmsg2 = new double[nslots];
    
    for(long i = 0; i < nslots; ++i){
        msg1[i] = 1.0/(i+1);
        msg2[i] = 1.0/(2*i+1);
        
        cmsg1[i].real(msg1[i]);
        cmsg2[i].real(msg2[i]);
    }
    
    Ciphertext ct1 = scheme.encrypt(cmsg1, nslots, pBits, logQ);
    Ciphertext ct2 = scheme.encrypt(cmsg2, nslots, pBits, logQ);
    

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed/2 << " s" << endl;
    cout << "------------------" << endl;
    
    
    /*---------------------------------------*/
    //  Rotation
    /*---------------------------------------*/
    long nshift = 1;
    Ciphertext ctrot;
    
    start= chrono::steady_clock::now();
    for(long i = 0; i < ntrial; ++i){
        ctrot = ct1;
        scheme.leftRotateAndEqual(ctrot, nshift);
    }

    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Rot time= " << timeElapsed/ntrial << " s" << endl;
    cout << "------------------" << endl;
    
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    
    cmsg1 = scheme.decrypt(secretKey, ctrot);
    for(long i = 0; i < nslots; ++i){
        dmsg1[i] = cmsg1[i].real();
    }
    
    double* msgrot = new double[nslots];
    long k = nslots - nshift;
    for(long j = 0; j < k; ++j){
        msgrot[j] = msg1[j + nshift];
    }
    for(long j = k; j < nslots; ++j){
        msgrot[j] = msg1[j - k];
    }
   
    for(long i = 0 ; i < 20; ++i){
        cout << i << ": " << msgrot[i] << ", " << dmsg1[i] << endl;
    }
     cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  Multiplication
    /*---------------------------------------*/
    
    Ciphertext ctmult;
    
    start= chrono::steady_clock::now();
    for(long i = 0; i < ntrial; ++i){
        ctmult = ct1;
        scheme.multAndEqual(ctmult, ct2);
    }
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Mult time= " << timeElapsed/ntrial << " s" << endl;
    cout << "------------------" << endl;
    
    scheme.reScaleByAndEqual(ctmult, pBits);
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    
    cmsg1 = scheme.decrypt(secretKey, ctmult);
    for(long i = 0; i < nslots; ++i){
        dmsg1[i] = cmsg1[i].real();
    }
    
    double* msgmult = new double[nslots];
    for(long i = 0; i < nslots; ++i){
        msgmult[i] = msg1[i] * msg2[i];
    }
    
    for(long i = 0 ; i < 20; ++i){
        cout << i << ": " << msgmult[i] << ", " << dmsg1[i] << endl;
    }
     cout << "------------------" << endl;
}


void TestHEmatrix::testEnc(long nrows) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);

    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Amat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j)%3)/10.0));
        }
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, Amat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> resmat;
    HEmatrix.decryptRmat(resmat, Actxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    
    RR error = getError(Amat, resmat, nrows, ncols);
    cout << "Error (enc): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(Amat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
}

void TestHEmatrix::testAdd(long nrows) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Mat<RR> Bmat;
    
    Amat.SetDims(nrows, ncols);
    Bmat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j) % 3) / 1.0));
            Bmat[i][j]= to_RR((((i*ncols + j ) % 3) / 1.0));
        }
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start = chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, Amat, HEmatpar.pBits);
    
    Ciphertext Bctxt;
    HEmatrix.encryptRmat(Bctxt, Bmat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    
    /*---------------------------------------*/
    //  Addition
    /*---------------------------------------*/
    
    start = chrono::steady_clock::now();
    scheme.addAndEqual(Actxt, Bctxt);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Add time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Actxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    add(resmat, Amat, Bmat);
    
    RR error = getError(resmat, HEresmat, nrows, ncols);
    cout << "Error (add): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}

void TestHEmatrix::testTrans(long nrows) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 35;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = lBits + cBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Amat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j)%3)/10.0));
        }
        Amat[i][i] += to_RR("2.0");
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, Amat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  Transposition
    /*---------------------------------------*/
    
    ZZX* transpoly;
    HEmatrix.genTransPoly(transpoly);
    
    Ciphertext Tctxt;
    
    start= chrono::steady_clock::now();
    
    HEmatrix.transpose(Tctxt, Actxt, transpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Trans time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Tctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    transpose(resmat, Amat);
    
    RR error = getError(resmat, HEresmat, nrows, ncols);
    cout << "Error (trans): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}


void TestHEmatrix::testShift(long nrows, long k) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = lBits + cBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Amat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j)%3)/10.0));
        }
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, Amat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  Transposition
    /*---------------------------------------*/
    
    ZZX* shiftpoly;
    HEmatrix.genShiftPoly(shiftpoly);
    
    Ciphertext Sctxt;
    
    start= chrono::steady_clock::now();
    
    HEmatrix.shiftBycols(Sctxt, Actxt, k, shiftpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Shift by" << k << " time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Sctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    cout << "------------------" << endl;
    cout << "Plaintext (before shifting)" << endl;
    printRmatrix(Amat, nrows);
    cout << "------------------" << endl;
    
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, nrows);
    cout << "------------------" << endl;
}


void TestHEmatrix::testMult(long nrows) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 14;   // scaling factor for constant
    long lBits = pBits + 8;
    long logQ = (2*cBits) + pBits + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Mat<RR> Bmat;
    
    Amat.SetDims(nrows, ncols);
    Bmat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j)%3) / 1.0));
            Bmat[i][j]= to_RR((((i*ncols + j )%3) / 1.0));
        }
    }
    cout << "Amat: " << Amat << endl;
    cout << "Bmat: " << Bmat << endl;
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, Amat, HEmatpar.pBits);
    
    Ciphertext Bctxt;
    HEmatrix.encryptRmat(Bctxt, Bmat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  GenPoly
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    
    ZZX** Initpoly;
    HEmatrix.genMultPoly(Initpoly);
    
    ZZX* shiftpoly;
    HEmatrix.genShiftPoly(shiftpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "GenPoly time= " << timeElapsed << " s" << endl;
    cout << "---------------------------" << endl;
    
    /*---------------------------------------*/
    //  Mult
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    Ciphertext Cctxt;
    HEmatrix.HEmatmul(Cctxt, Actxt, Bctxt, Initpoly, shiftpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Mult time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Cctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    mul(resmat, Amat, Bmat);
    
    RR error = getError(resmat, HEresmat, nrows, ncols);
    cout << "Error (mul): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}


void TestHEmatrix::testRMult(long nrows, long subdim) {
  
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 14;   // scaling factor for constant
    long lBits = pBits + 8;
    long logQ = (2*cBits) + pBits + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, subdim);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> rAmat;
    rAmat.SetDims(subdim, ncols);
    
    Mat<RR> Bmat;
    Bmat.SetDims(nrows, ncols);
    
    for(long i = 0; i < subdim; i++){
        for(long j = 0; j < ncols; j++){
            rAmat[i][j]= to_RR((((i + ncols * j)%3)/10.0));
        }
    }
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Bmat[i][j]= to_RR((((i*ncols + j )%3)/10.0));
        }
    }
    
    // replicate
    Mat<RR> rAmat1;
    rAmat1.SetDims(nrows, ncols);
    for(long i = 0; i < nrows/subdim; ++i){
        for(long j = 0; j < subdim; ++j){
            for(long k = 0; k< ncols; k++){
                rAmat1[i*subdim + j][k] = rAmat[j][k];
            }
        }
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext Actxt;
    HEmatrix.encryptRmat(Actxt, rAmat1, HEmatpar.pBits);
    
    Ciphertext Bctxt;
    HEmatrix.encryptRmat(Bctxt, Bmat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  GenPoly
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    
    ZZX** Initpoly;
    HEmatrix.genMultPoly(Initpoly);
    
    ZZX* shiftpoly;
    HEmatrix.genShiftPoly(shiftpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "GenPoly time= " << timeElapsed << " s" << endl;
    cout << "---------------------------" << endl;
    
    /*---------------------------------------*/
    //  Mult
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    Ciphertext Cctxt;
    HEmatrix.HErmatmul(Cctxt, Actxt, Bctxt, Initpoly, shiftpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "R-Mult time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Cctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    mul(resmat, rAmat, Bmat);
    
    RR error = getError(resmat, HEresmat, subdim, ncols);
    cout << "Error (mul): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}

// If Amat is given in plaintext

void TestHEmatrix::testMult_preprocessing(long nrows) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 14;   // scaling factor for constant
    long lBits = pBits + 8;
    long logQ = cBits + pBits + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> Amat;
    Mat<RR> Bmat;
    
    Amat.SetDims(nrows, ncols);
    Bmat.SetDims(nrows, ncols);
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Amat[i][j]= to_RR((((i* 2 + ncols * j)%3)/10.0));
            Bmat[i][j]= to_RR((((i*ncols + j )%3)/10.0));
        }
    }
    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext* Actxts;
    HEmatrix.genInitActxt(Actxts, Amat);
    
    Ciphertext Bctxt;
    HEmatrix.encryptRmat(Bctxt, Bmat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  GenPoly
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    
    ZZX* Initpoly;
    HEmatrix.genMultBPoly(Initpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "GenPoly time= " << timeElapsed << " s" << endl;
    cout << "---------------------------" << endl;
    
    /*---------------------------------------*/
    //  Mult
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    Ciphertext Cctxt;
    HEmatrix.HEmatmul_preprocessing(Cctxt, Actxts, Bctxt, Initpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Mult time (preprocessing)= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Cctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    mul(resmat, Amat, Bmat);
    
    RR error = getError(resmat, HEresmat, nrows, ncols);
    cout << "Error (mul): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}

void TestHEmatrix::testRMult_preprocessing(long nrows, long subdim) {
    
    long logN = 13;
    
    long ncols = nrows;
    long pBits = 25;   // scaling factor for message
    long cBits = 14;   // scaling factor for constant
    long lBits = pBits + 8;
    long logQ = (cBits) + pBits + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, subdim);
    
    cout << "------------------" << endl;
    cout << "(dim,po2dim, nslots) = (" << nrows << "," << HEmatpar.dim  << "," << HEmatpar.sqrdim << "," << HEmatpar.nslots << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR> rAmat;
    rAmat.SetDims(subdim, ncols);
    
    Mat<RR> Bmat;
    Bmat.SetDims(nrows, ncols);
    
    for(long i = 0; i < subdim; i++){
        for(long j = 0; j < ncols; j++){
            rAmat[i][j]= to_RR((((i + ncols * j)%3)/10.0));
        }
    }
    
    for(long i = 0; i < nrows ; i++){
        for(long j = 0; j < ncols; j++){
            Bmat[i][j]= to_RR((((i*ncols + j )%3)/10.0));
        }
    }

    
    /*---------------------------------------*/
    //  Encryption
    /*---------------------------------------*/
    
    auto start= chrono::steady_clock::now();
    Ciphertext* Actxts;
    HEmatrix.genInitRecActxt(Actxts, rAmat);
    
    Ciphertext Bctxt;
    HEmatrix.encryptRmat(Bctxt, Bmat, HEmatpar.pBits);
    
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "Enc time= " << timeElapsed << " s" << endl;
    cout << "------------------" << endl;
    
    /*---------------------------------------*/
    //  GenPoly
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    
    ZZX* Initpoly;
    HEmatrix.genMultBPoly(Initpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "GenPoly time= " << timeElapsed << " s" << endl;
    cout << "---------------------------" << endl;
    
    /*---------------------------------------*/
    //  Mult
    /*---------------------------------------*/
    
    start= chrono::steady_clock::now();
    Ciphertext Cctxt;
    HEmatrix.HErmatmul_preprocessing(Cctxt, Actxts, Bctxt, Initpoly);
    
    end = std::chrono::steady_clock::now();
    diff = end - start;
    timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
    cout << "R-Mult time= " << timeElapsed << " s" << endl;
    
    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> HEresmat;
    HEmatrix.decryptRmat(HEresmat, Cctxt);
    
    /*---------------------------------------*/
    //  Error
    /*---------------------------------------*/
    Mat<RR> resmat;
    mul(resmat, rAmat, Bmat);
    
    RR error = getError(resmat, HEresmat, subdim, ncols);
    cout << "Error (mul): " << error << endl;
    
    cout << "------------------" << endl;
    cout << "Plaintext" << endl;
    printRmatrix(resmat, 4);
    cout << "------------------" << endl;
    cout << "Encryption" << endl;
    printRmatrix(HEresmat, 4);
    cout << "------------------" << endl;
}

//----------------------------------------------------
// SIMD (parallel computation)

void TestHEmatrix::testSIMDAdd(long nrows, long nbatching, const long niter) {
    
    long logN = 13;
    
    long ncols = nrows;
    
    if(nrows * ncols * nbatching > (1<< (logN-1))){
        cout << "Cannot support the parallism " << endl;
    }
    
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, 0, nbatching);
    
    cout << "------------------" << endl;
    cout << "(dim, nslots, nbatching) = (" << nrows << ","  << HEmatpar.nslots << ","  << HEmatpar.nbatching << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR>* Amat = new Mat<RR>[nbatching];
    Mat<RR>* Bmat = new Mat<RR>[nbatching];
    
    for(long k = 0; k < nbatching; ++k){
        Amat[k].SetDims(nrows, ncols);
        Bmat[k].SetDims(nrows, ncols);
        
        for(long i = 0; i < nrows ; i++){
            for(long j = 0; j < ncols; j++){
                //Amat[k][i][j] = to_RR((((i*2 + ncols * j + 1)%3)/10.0));
                Amat[k][i][j] = to_RR(((i*ncols + j)%3/10.0));
                Bmat[k][i][j] = to_RR(((i*ncols + j)%3/10.0));
            }
        }
    }
    
    double totalEnctime = 0.0;
    double totalEvaltime = 0.0;
    double amortizedtime = 0.0;
    double totalDectime = 0.0;
    
    for(long l = 0; l < niter; ++l){
        /*---------------------------------------*/
        //  Encryption
        /*---------------------------------------*/
        
        auto start= chrono::steady_clock::now();
        Ciphertext Actxt;
        HEmatrix.encryptParallelRmat(Actxt, Amat, HEmatpar.pBits, nbatching);
        
        Ciphertext Bctxt;
        HEmatrix.encryptParallelRmat(Bctxt, Bmat, HEmatpar.pBits, nbatching);
        
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalEnctime += timeElapsed;
        
        cout << "Encryption" << endl;
        
        /*---------------------------------------*/
        //  Addition
        /*---------------------------------------*/
        
        start= chrono::steady_clock::now();
        
        scheme.addAndEqual(Actxt, Bctxt);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        
        totalEvaltime += timeElapsed;
        amortizedtime += timeElapsed/nbatching;
        
        /*---------------------------------------*/
        //  Decryption
        /*---------------------------------------*/
        start= chrono::steady_clock::now();
        
        Mat<RR>* HEresmat;
        HEmatrix.decryptParallelRmat(HEresmat, Actxt);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalDectime += timeElapsed;
        
        /*---------------------------------------*/
        //  Error
        /*---------------------------------------*/
        RR avgerror_add = to_RR("0");
        
        Mat<RR>* resmat = new Mat<RR>[nbatching];
        for(long k = 0; k < nbatching; ++k){
            add(resmat[k], Amat[k], Bmat[k]);
            avgerror_add += getError(resmat[k], HEresmat[k], nrows, ncols);
        }
        avgerror_add /= nbatching;
        
        if(l == niter -1) cout << "Error (add): " << avgerror_add << endl;
    }
    
    cout << "------------------" << endl;
    cout << "Enc time= " << totalEnctime/niter << " s" << endl;
    cout << "Add time= " << totalEvaltime/niter << " s/ " ;
    cout << "Amortized time= " << amortizedtime/niter << " s" << endl;
    cout << "Dec time= " <<  totalDectime/niter << " s" << endl;
    
}

void TestHEmatrix::testSIMDTrans(long nrows, long nbatching, const long niter) {
    
    long logN = 13;
    
    long ncols = nrows;
    
    if(nrows * ncols * nbatching > (1<< (logN-1))){
        cout << "Cannot support the parallism " << endl;
    }
    
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = cBits + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, 0, nbatching);
    
    cout << "------------------" << endl;
    cout << "(dim, nslots, nbatching) = (" << nrows << ","  << HEmatpar.nslots << ","  << HEmatpar.nbatching << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR>* Amat = new Mat<RR>[nbatching];
   
    for(long k = 0; k < nbatching; ++k){
        Amat[k].SetDims(nrows, ncols);
        for(long i = 0; i < nrows ; i++){
            for(long j = 0; j < ncols; j++){
                Amat[k][i][j] = to_RR(((i*ncols + j)%3/10.0));
            }
        }
    }
    
    double totalEnctime = 0.0;
    double totalEvaltime = 0.0;
    double amortizedtime = 0.0;
    double totalDectime = 0.0;
    
    for(long l = 0; l < niter; ++l){
        /*---------------------------------------*/
        //  Encryption
        /*---------------------------------------*/
        
        auto start= chrono::steady_clock::now();
        Ciphertext Actxt;
        HEmatrix.encryptParallelRmat(Actxt, Amat, HEmatpar.pBits, nbatching);
        
        
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalEnctime += timeElapsed;
        
        
        /*---------------------------------------*/
        //  Transpose
        /*---------------------------------------*/
        ZZX* transpoly;
        HEmatrix.genTransPoly_Parallel(transpoly);
        
        start= chrono::steady_clock::now();
        Ciphertext Tctxt;
        HEmatrix.transpose_Parallel(Tctxt, Actxt, transpoly);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        
        totalEvaltime += timeElapsed;
        amortizedtime += timeElapsed/nbatching;
        
        /*---------------------------------------*/
        //  Decryption
        /*---------------------------------------*/
        start= chrono::steady_clock::now();
        
        Mat<RR>* HEresmat;
        HEmatrix.decryptParallelRmat(HEresmat, Tctxt);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalDectime += timeElapsed;
        
        /*---------------------------------------*/
        //  Error
        /*---------------------------------------*/
        RR avgerror = to_RR("0");
        Mat<RR>* resmat = new Mat<RR>[nbatching];
        for(long k = 0; k < nbatching; ++k){
            transpose(resmat[k], Amat[k]);
            avgerror += getError(HEresmat[k], resmat[k], nrows, ncols);
        }
        avgerror /= nbatching;
        
        if(l == niter -1) cout << "Error (trans): " << avgerror << endl;
    }
    
    cout << "------------------" << endl;
    cout << "Enc time= " << totalEnctime/niter << " s" << endl;
    cout << "Trans time= " << totalEvaltime/niter << " s/ " ;
    cout << "Amortized time= " << amortizedtime/niter << " s" << endl;
    cout << "Dec time= " <<  totalDectime/niter << " s" << endl;
    
}


void TestHEmatrix::testSIMDMult(long nrows, long nbatching, const long niter) {
    
    long logN = 13;
    
    long ncols = nrows;
    
    if(nrows * ncols * nbatching > (1<< (logN-1))){
        cout << "Cannot support the parallism " << endl;
    }
    
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = (2*cBits + pBits) + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, 0, nbatching);
    
    cout << "------------------" << endl;
    cout << "(dim, nslots, nbatching) = (" << nrows << ","  << HEmatpar.nslots << ","  << HEmatpar.nbatching << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR>* Amat = new Mat<RR>[nbatching];
    Mat<RR>* Bmat = new Mat<RR>[nbatching];
    
    for(long k = 0; k < nbatching; ++k){
        Amat[k].SetDims(nrows, ncols);
        Bmat[k].SetDims(nrows, ncols);
        
        for(long i = 0; i < nrows ; i++){
            for(long j = 0; j < ncols; j++){
                //Amat[k][i][j] = to_RR((((i*2 + ncols * j + 1)%3)/10.0));
                Amat[k][i][j] = to_RR(((i*ncols + j)%3/10.0));
                Bmat[k][i][j] = to_RR(((i*ncols + j)%3/10.0));
            }
        }
    }
    
    double totalEnctime = 0.0;
    double totalEvaltime = 0.0;
    double amortizedtime = 0.0;
    double totalDectime = 0.0;
    double genpolytime = 0.0;
    
    for(long l = 0; l < niter; ++l){
        /*---------------------------------------*/
        //  Encryption
        /*---------------------------------------*/
        
        auto start= chrono::steady_clock::now();
        Ciphertext Actxt;
        HEmatrix.encryptParallelRmat(Actxt, Amat, HEmatpar.pBits, nbatching);
        
        Ciphertext Bctxt;
        HEmatrix.encryptParallelRmat(Bctxt, Bmat, HEmatpar.pBits, nbatching);
        
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalEnctime += timeElapsed;
        
        /*---------------------------------------*/
        //  Genpoly
        /*---------------------------------------*/
        
        start= chrono::steady_clock::now();
        
        ZZX** Initpoly;
        HEmatrix.genMultPoly_Parallel(Initpoly);
        
        ZZX* shiftpoly;
        HEmatrix.genShiftPoly_Parallel(shiftpoly);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        genpolytime += timeElapsed;
        
        /*---------------------------------------*/
        //  Mult
        /*---------------------------------------*/
        
        start= chrono::steady_clock::now();
        
        Ciphertext Cctxt;
        HEmatrix.HEmatmul_Parallel(Cctxt, Actxt, Bctxt, Initpoly, shiftpoly);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        
        totalEvaltime += timeElapsed;
        amortizedtime += timeElapsed/nbatching;
        
        /*---------------------------------------*/
        //  Decryption
        /*---------------------------------------*/
        start= chrono::steady_clock::now();
        
        Mat<RR>* HEresmat;
        HEmatrix.decryptParallelRmat(HEresmat, Cctxt);
        
        end = std::chrono::steady_clock::now();
        diff = end - start;
        timeElapsed = chrono::duration <double, milli> (diff).count()/1000.0;
        totalDectime += timeElapsed;
        
        /*---------------------------------------*/
        //  Error
        /*---------------------------------------*/
        RR avgerror_mult = to_RR("0");
        Mat<RR>* resmat = new Mat<RR>[nbatching];
        for(long k = 0; k < nbatching; ++k){
            mul(resmat[k], Amat[k], Bmat[k]);
            avgerror_mult += getError(HEresmat[k], resmat[k], nrows, ncols);
        }
        avgerror_mult /= nbatching;
        
        if(l == niter -1) cout << "Error (mult): " << avgerror_mult << endl;
    }
    
    cout << "------------------" << endl;
    cout << "Enc time= " << totalEnctime/niter << " s" << endl;
    cout << "Mult time= " << totalEvaltime/niter << " s/ " ;
    cout << "Amortized time= " << amortizedtime/niter << " s" << endl;
    cout << "Dec time= " <<  totalDectime/niter << " s" << endl;
    
}

void TestHEmatrix::testSIMDMult_Huang(long Arows, long Acols, long Brows, long Bcols, long nbatching)
{

    long logN = 13;
    
    long ncols = Bcols;
    long nrows = Bcols;

    if(nrows * ncols * nbatching > (1<< (logN-1)))
    {
        cout << "Cannot support the parallism " << endl;
    }
    
    long pBits = 25;   // scaling factor for message
    long cBits = 15;   // scaling factor for constant
    long lBits = pBits + 5;
    long logQ = (2*cBits + pBits) + lBits;
    
    long h = 64;
    long keydist = 1;
    
    struct HEMatpar HEmatpar;
    readHEMatpar(HEmatpar, nrows, ncols, pBits, cBits, logQ, 0, nbatching);
    
    cout << "------------------" << endl;
    cout << "(dim, nslots, nbatching) = (" << nrows << ","  << HEmatpar.nslots << ","  << HEmatpar.nbatching << ")" << endl;
    cout << "HEAAN PARAMETER logQ: " << logQ << endl;
    cout << "HEAAN PARAMETER logN: " << logN << endl;
    
    /*---------------------------------------*/
    //  Initialization random seed
    /*---------------------------------------*/
    srand(time(NULL));
    /*---------------------------------------*/
    //  Key Generation
    /*---------------------------------------*/
    
    TimeUtils timeutils;
    timeutils.start("Scheme generating...");
    Context context(logN, logQ);
    SecretKey secretKey(logN, h);
    Scheme scheme(secretKey, context);
    
    scheme.addLeftRotKeys(secretKey);
    scheme.addRightRotKeys(secretKey);
    timeutils.stop("Scheme generation");
    
    HEmatrix HEmatrix(scheme, secretKey, HEmatpar);
    
    /*---------------------------------------*/
    //  Generate a random matrix
    /*---------------------------------------*/
    Mat<RR>* Mat_Vertical_Stack = new Mat<RR>[2];
    Mat_Vertical_Stack[0].SetDims(Acols, Acols);Mat_Vertical_Stack[1].SetDims(Acols, Acols);
    Mat<RR>* Mat_Horizontal_Stack = new Mat<RR>[2];
    Mat_Horizontal_Stack[0].SetDims(Brows, Brows);Mat_Horizontal_Stack[1].SetDims(Brows, Brows);

    Mat<RR> vertical_Amat, Amat, Random_Amat;
    vertical_Amat.SetDims(2 * Arows, Acols);Amat.SetDims(Acols, Acols);Random_Amat.SetDims(Acols, Acols);
    Mat<RR> horizontal_Bmat_trans,Bmat, Random_Bmat;
    horizontal_Bmat_trans.SetDims(2 * Brows, Brows);Bmat.SetDims(Brows, Brows);Random_Bmat.SetDims(Brows, Brows);
    
    for(long i = 0; i < Acols; i++)
    {
        for(long j = 0; j < Acols; j++)
        {
            Amat[i][j] = to_RR(rand() % 5);
            Random_Amat[i][j] = to_RR(rand() % 5);
        }
        vertical_Amat[i] = Amat[i];
        vertical_Amat[i + Acols] = Random_Amat[i];
    }

    cout << "Amat : " << endl;
    cout << Amat << endl;
    cout << "Random_Amat : " << endl;
    cout << Random_Amat << endl;
    cout << "vertical_Amat : " << endl;
    cout << vertical_Amat << endl;

    for(long i = 0; i < Bcols; i++)
    {
        for(long j = 0; j < Bcols; j++)
        {
            Bmat[i][j] = to_RR(rand() % 5);
            Random_Bmat[i][j] = to_RR(rand() % 5);
        }
    }
    Mat<RR> Bmat_trans = transpose(Bmat);
    Mat<RR> Random_Bmat_trans = transpose(Random_Bmat);
    for(long j = 0; j < Bcols; j++)
    {
        horizontal_Bmat_trans[j] = Bmat_trans[j];
        horizontal_Bmat_trans[j + Bcols] = Random_Bmat_trans[j];
    }

    cout << "Bmat : " << endl;
    cout << Bmat << endl;
    cout << "Random_Bmat : " << endl;
    cout << Random_Bmat << endl;
    cout << "horizontal_Bmat : " << endl;
    cout << horizontal_Bmat_trans << endl;

    Mat<RR> permutation_Amat, permutation_Bmat;
    permutation_Amat.SetDims(2 * Arows, 2 * Arows);
    permutation_Bmat.SetDims(2 * Bcols, 2 * Bcols);
    generate_random_permutation_matrix(permutation_Amat);   // ??? 
    generate_random_permutation_matrix(permutation_Bmat);

    cout << "permutation_Amat : " << endl;
    cout << permutation_Amat << endl;

    cout << "permutation_Bmat : " << endl;
    cout << permutation_Bmat << endl;

    Mat<RR> vertical_Amat_mul = permutation_Amat * vertical_Amat;
    Mat<RR> horizontal_Bmat_mul = transpose(horizontal_Bmat_trans) * permutation_Bmat;


    for(long i = 0; i < Arows; i++)
    {
        Mat_Vertical_Stack[0][i] = vertical_Amat_mul[i];
        Mat_Vertical_Stack[1][i] = vertical_Amat_mul[i + Arows];
    }
    for(long i = 0; i < Arows; i++)
    {
        for(long j = 0; j < Arows; j++)
        {
            Mat_Horizontal_Stack[0][i][j] = horizontal_Bmat_mul[i][j];
            Mat_Horizontal_Stack[1][i][j] = horizontal_Bmat_mul[i][j + Arows];
        }
    }


    cout << "Mat_Vertical_Stack[0] : " << endl;
    cout << Mat_Vertical_Stack[0] << endl;
    cout << "Mat_Vertical_Stack[1] : " << endl;
    cout << Mat_Vertical_Stack[1] << endl;

    cout << "transpose Mat_Horizontal_Stack[0] : " << endl;
    cout << Mat_Horizontal_Stack[0] << endl;

    cout << "transpose Mat_Horizontal_Stack[1] : " << endl;
    cout << Mat_Horizontal_Stack[1] << endl;

    /* 0th diagonal multiply vector */
    Ciphertext Actxt;
    HEmatrix.encryptParallelRmat(Actxt, Mat_Vertical_Stack, HEmatpar.pBits, nbatching);

    Ciphertext Bctxt;
    HEmatrix.encryptParallelRmat(Bctxt, Mat_Horizontal_Stack, HEmatpar.pBits, nbatching);
    
    ZZX** Initpoly;
    HEmatrix.genMultPoly_Parallel_Huang(Initpoly, 0);

    ZZX* shiftpoly;
    HEmatrix.genShiftPoly_Parallel(shiftpoly);

    Ciphertext* res = new Ciphertext[nbatching];
    HEmatrix.HEmatmul_Parallel_Huang(res[0], Actxt, Bctxt, Initpoly, shiftpoly);

    /* 1th diagonal multiply vector */
    Ciphertext Bctxt_rot_one = scheme.leftRotate(Bctxt, 1);

    HEmatrix.genMultPoly_Parallel_Huang(Initpoly, 1);

    HEmatrix.HEmatmul_Parallel_Huang(res[1], Actxt, Bctxt_rot_one, Initpoly, shiftpoly);

    /*---------------------------------------*/
    //  Decryption
    /*---------------------------------------*/
    Mat<RR> DecryptionRes;DecryptionRes.SetDims(2 * Arows, 2 * Arows);
    Mat<RR>** HEresmat = new Mat<RR>*[nbatching];
    for(long i = 0; i < nbatching; i++)
    {
        HEmatrix.decryptParallelRmat(HEresmat[i], res[i]);
        cout << "(" << i << ",0)" << endl;
        cout << HEresmat[i][0] << endl;
        cout << "(" << i << ",1)" << endl;
        cout << HEresmat[i][1] << endl;
        for(long j = 0; j < nbatching; j++)
        {
            for(long k = 0; k < Arows; k++)
            {
                for(long l = 0; l < Arows; l++)
                {
                    DecryptionRes[(k + j * Arows)][(l + (i + j) * Arows) % (2 * Arows)] = HEresmat[i][j][k][l];
                }
            }
            
        }
    }
    cout << "DecryptionRes : " << endl;
    cout << DecryptionRes << endl;
    // plaintext random matrix multiplication

    Mat<RR> permutation_res = transpose(permutation_Amat) * DecryptionRes * transpose(permutation_Bmat);
    Mat<RR> random_matrix_multiplication = Random_Amat * Random_Bmat;

    cout << "permutation_res : " << endl;
    cout << permutation_res << endl;
    cout << "random_matrix_multiplication : " << endl;
    cout << random_matrix_multiplication << endl;
    bool flag = false; 
    for(long i = Arows; i < 2 * Arows; i++)
    {
        for(long j = Arows; j < 2 * Arows; j++)
        {
            if(abs(permutation_res[i][j] - random_matrix_multiplication[i - Arows][j - Arows]) > to_RR(0.01))
            {
                flag = true;
                cout << "The result is not correct !!!" << endl;
                break;
            }
        }
    }
    if(!flag)
    {
        cout << "The result is correct" << endl;
    }
}
