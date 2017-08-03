//����LU - SGS����������DU = U(n + 1) - U(n)

#include <stdlib.h>
#include<stdio.h>
#include "sub_LU_SGS.h"
#include "Global_var.h"
#include "common.h"


void comput_DFn(int NV, double*  DF, double U[], double DU[], double n1, double  n2, double gamma);

__global__
void sweep(int offset, int * transferInt, double *transferDouble, double * dU)
{


}

//����LU_SGS��������DU = U(n + 1) - U(n)
void  du_LU_SGS_2D(int nMesh, int mBlock, double alfa1)
{
	double alfa[7], dui[7], duj[7], DF[7];   //alfa, �Խ���Ԫ��ֵ;  dui, duj, DF����ͨ��
	const double w_LU = 1.E0;   //LU_SGS���ɳ�����(1 - 2֮��), ����w_LU������ȶ��ԣ����ή�������ٶ�
	alfa1;                 //˫ʱ�䲽LU_SGS���ӶԽ���ֵ
	Mesh_TYPE & MP = Mesh[nMesh];
	int NV = MP.Nvar;
	Block_TYPE & B = MP.Block[mBlock];                 //��nMesh ������ĵ�mBlock��
	int nx = B.nx; int ny = B.ny;

	//LU - SGS������ɨ��
	//----------------------------------
	//��i = 1, j = 1 ��i = nx - 1, j = ny - 1��ɨ�����(����ɨ�����)
	//ɨ�� i + j = plane ��ƽ��
	//w_LU���ɳ����ӣ�1��2֮�䣩������w_LU������ȶ��ԣ����ή�������ٶ�
	for (int plane = 2; plane <= nx + ny - 2; ++plane) {
		//$OMP PARALLEL for( int DEFAULT(PRIVATE) SHARED(plane, nx, ny, NV, B, gamma, If_viscous, alfa1)
		int maxCirculation = plane - 1 < ny - 1 ? plane + 1 : ny - 1;
		int minCirculation = plane - nx + 1 > 1 ? plane - nx + 1 : 1;
		int offset = minCirculation;
		int total_threads = maxCirculation - minCirculation + 1;
		int thread = 32;
		int block = (total_threads + thread-1) / total_threads;

		sweep << <block, thread >> > (offset, transferInt_dev, transferDouble_dev, dU);
		for (int j = 1; j <= ny - 1; ++j) {
			int i = plane - j;
			if (i< 1 || i>nx - 1) { continue; }    //���������ƽ��
												   //����Խ���Ԫ�� ���翼��ճ�����ټ���ճ���װ뾶�� ��������ģ���еķ��̣�������Դ���װ뾶��
			for (int m = 1; m <= 6; ++m) {
				alfa[m] = B.vol[i][j] / B.dt[i][j] + w_LU*(B.Lci[i][j] + B.Lcj[i][j]) + B.vol[i][j] * alfa1;        //�Խ�Ԫ
			}

			if (If_viscous == 1) {
				for (int m = 1; m <= 6; ++m) {
					alfa[m] += 2.E0*(B.Lvi[i][j] + B.Lvj[i][j]);           //����ճ����
				}
			}
			if (NV == 5) {
				alfa[5] = alfa[5] + B.Lvi[i][j] + B.Lvj[i][j];                //SAģ��
																			  //}elseif(NV== 6) {                                 //SSTģ��
																			  //alfa(5) = alfa(5) + 0.09*B.U(6[i][j] / B.U(1[i][j]
																			  //alfa(6) = alfa(6) + 2.E0*0.0828*B.U(6[i][j] / B.U(1[i][j]
			}

			if (i != 1) {
				//ͨ���Ĳ������������Ƽ���A*W(See Blazek's book, page 208)
				comput_DFn(NV, DF, B.U[i - 1 + LAP][j + LAP], B.dU[i - 1][j], B.ni1[i][j], B.ni2[i][j], gamma);

				for (int m = 1; m <= NV; ++m) {
					dui[m] = 0.5E0*(DF[m] * B.si[i][j] + w_LU*B.Lci[i - 1][j] * B.dU[i - 1][j][m]);
				}

				if (If_viscous == 1) {
					for (int m = 1; m <= NV; ++m) {
						dui[m] = dui[m] + B.Lvi[i - 1][j] * B.dU[i - 1][j][m];         //2012 - 2 - 29, �ȶ��Ը���Щ
					}
				}

			}
			else {
				for (int m = 1; m <= NV; ++m) {
					dui[m] = 0.E0;                             //���û�е�
				}
			}

			if (j != 1) {
				comput_DFn(NV, DF, B.U[i+LAP][j - 1+LAP], B.dU[i][j - 1], B.nj1[i][j], B.nj2[i][j], gamma);
				for (int m = 1; m <= NV; ++m) {
					duj[m] = 0.5E0*(DF[m] * B.sj[i][j] + w_LU*B.Lcj[i][j - 1] * B.dU[i][j - 1][m]);
				}

				if (If_viscous == 1) {
					for (int m = 1; m <= NV; ++m) {
						duj[m] = duj[m] + B.Lvj[i][j - 1] * B.dU[i][j - 1][m];    //2012 - 2 - 29
					}
				}
			}
			else {
				for (int m = 1; m <= NV; ++m) {
					duj[m] = 0.E0;
				}
			}
			//printf("%d , %d \n", i, j);
			for (int m = 1; m <= NV; ++m) {
				B.dU[i][j][m] = (B.Res[i][j][m] + dui[m] + duj[m]) / alfa[m];
				//printf("%f,  %f,  %f,  %f,   %f\n", B.dU[i][j][m], B.Res[i][j][m], dui[m], duj[m], alfa[m]);
			}
			//PAUSE;
		}
		//$OMP END PARALLEL do int
	}
	//----------------------------------------------------------
	//��(nx - 1, ny - 1)��(1, 1)��ɨ����� ������ɨ����̣�
	//plane = i + j + k
	for (int plane = nx + ny - 2; plane >= 2; --plane) {

		//$OMP PARALLEL for( int DEFAULT(PRIVATE) SHARED(plane, nx, ny, NV, B, gamma, If_viscous, alfa1)
		for (int j = ny - 1; j >= 1; --j) {
			int i = plane - j;
			if (i< 1 || i>nx - 1) { continue; }            //���������ƽ��
			for (int m = 1; m <= 6; ++m) {
				alfa[m] = B.vol[i][j] / B.dt[i][j] + w_LU*(B.Lci[i][j] + B.Lcj[i][j]) + B.vol[i][j] * alfa1;
			}
			if (If_viscous == 1) {
				for (int m = 1; m <= 6; ++m) {
					alfa[m] += 2.E0*(B.Lvi[i][j] + B.Lvj[i][j]);
				}
			}
			if (NV == 5) {
				alfa[5] = alfa[5] + B.Lvi[i][j] + B.Lvj[i][j];
				//}elseif(NV== 6) {
				//alfa(5) = alfa(5) + 0.09*B.U(6[i][j] / B.U(1[i][j]
				//alfa(6) = alfa(6) + 2.E0*0.0828*B.U(6[i][j] / B.U(1[i][j]
			}

			if (i != nx - 1) {
				//ͨ���Ĳ������������Ƽ���A*W(See Blazek's book, page 208)
				comput_DFn(NV, DF, B.U[i + 1+LAP][j+LAP], B.dU[i + 1][j], B.ni1[i][j], B.ni2[i][j], gamma);
				for (int m = 1; m <= NV; ++m) {
					dui[m] = -0.5E0*(DF[m] * B.si[i + 1][j] - w_LU*B.Lci[i + 1][j] * B.dU[i + 1][j][m]);
				}
				if (If_viscous == 1) {
					for (int m = 1; m <= NV; ++m) {
						dui[m] += B.Lvi[i + 1][j] * B.dU[i + 1][j][m];
					}
				}
			}
			else {
				for (int m = 1; m <= NV; ++m) {
					dui[m] = 0.E0;
				}
			}

			if (j != ny - 1) {
				comput_DFn(NV, DF, B.U[i+LAP][j + 1+LAP], B.dU[i][j + 1], B.nj1[i][j], B.nj2[i][j], gamma);
				for (int m = 1; m <= NV; ++m) {
					duj[m] = -0.5E0*(DF[m] * B.sj[i][j + 1] - w_LU*B.Lcj[i][j + 1] * B.dU[i][j + 1][m]);
				}
				if (If_viscous == 1) {
					for (int m = 1; m <= NV; ++m) {
						duj[m] = duj[m] + B.Lvj[i][j + 1] * B.dU[i][j + 1][m];
					}
				}
			}
			else {
				for (int m = 1; m <= NV; ++m) {
					duj[m] = 0.E0;
				}
			}

			for (int m = 1; m <= NV; ++m) {
				B.dU[i][j][m] += (dui[m] + duj[m]) / alfa[m];
			}
		}
		//$OMP END PARALLEL for( int
	}
}


//����ͨ���Ĳ��� DF = F(U + DU) - F(U), LU - SGS������ʹ�ã���������A*DU
//comput_DFn(NV, DF, B.U[i - 1 + LAP][j + LAP], B.dU[i - 1][j], B.ni1[i][j], B.ni2[i][j], gamma);
void comput_DFn(int NV, double*  DF, double U[], double DU[], double n1, double  n2, double gamma)
{
	double * U2=(double *) malloc((NV+1)*sizeof(double));
	for (int m = 1; m <= NV; ++m) {
		U2[m] = U[m] + DU[m];		//�µ��غ����
		//printf("U2= %f    %d \n", U2[m], m);
	}
	double un1 = (U[2] * n1 + U[3] * n2) / U[1];      //un �����ٶ�
	double p1 = (gamma - 1.e0)*(U[4] - 0.5e0*(U[2] * U[2] + U[3] * U[3]) / U[1]);   //ѹ��
	double un2 = (U2[2]*n1 + U2[3]*n2) / U2[1];       //�����ٶ�un
	double p2 = (gamma - 1.e0)*(U2[4] - 0.5e0*(U2[2] * U2[2] + U2[3] * U2[3]) / U2[1]);   //ѹ��
	
	//ͨ��֮�� DF = F(U + DU) - F(U)
	DF[1] = U2[1] * un2 - U[1] * un1;                  //d*un
	DF[2] = (U2[2] * un2 + p2*n1) - (U[2] * un1 + p1*n1);
	DF[3] = (U2[3] * un2 + p2*n2) - (U[3] * un1 + p1*n2);
	DF[4] = (U2[4] + p2) * un2 - (U[4] + p1) * un1;
	if (NV == 5) {
		DF[5] = U2[5] * un2 - U[5] * un1;
	}
	else if(NV ==  6){
		DF[5] = U2[5] * un2 - U[5] * un1;   //k���̵Ķ���ͨ��
		DF[6] = U2[6] * un2 - U[6] * un1;   //w���̵Ķ���ͨ��
	}
	free(U2);
}
