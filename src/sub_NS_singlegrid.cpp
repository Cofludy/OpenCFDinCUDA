#include "Global_var.h"
#include "sub_NS_singlegid.h"
#include "sub_Residual.h"
#include "sub_boundary.h"
#include "Flow_Var.h"
#include "sub_LU_SGS.h"
#include "sub_NS_multigrid.h"
#include <stdio.h>
#include <cmath>
#include <ctime>
#include <fstream>

void force_vt_kw(int nMesh);

//�ڸ��������������N - S���� ���ƽ�1��ʱ�䲽��
//���ڵ�������nMesh = 1;  ���ڶ�������nMesh = 1, 2, 3, ... �ֱ��Ӧ��ϸ���񡢴����񡢸������� ...
void NS_Time_advance(int nMesh)
{
	if (Time_Method == Time_Euler1) {	 //1��Euler
		NS_Time_advance_1Euler(nMesh);          
	}
	else if (Time_Method == Time_RK3) {		 //3��RK
		//NS_Time_advance_RK3(nMesh);                      
	}
	else if (Time_Method == Time_LU_SGS) {			//LU - SGS����ʽ
		NS_time_advance_LU_SGS(nMesh);
	}
	else if (Time_Method == Time_Dual_LU_SGS) {	//˫ʱ�䲽LU - SGS(��֧�ֶ�������)
		//Dual_time_LU_SGS();
	}
	else {
		printf("This time advance method is not supported!!!\n");
	}

	//ǿ�� k, w, vt �Ǹ�
	force_vt_kw(nMesh);
}


//ǿ�� k, w, vt �Ǹ�
void force_vt_kw(int nMesh)
{
	Mesh_TYPE & MP = Mesh[nMesh];
	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
		Block_TYPE & B = MP.Block[mBlock];
		int nx = B.nx;  int ny = B.ny;
		if (MP.Nvar == 5) {
			for (int i = 1+LAP; i <= nx - 1+LAP; ++i) {
				for (int j = 1+LAP; j <= ny - 1+LAP; ++j) {
					if (B.U[i][j][5] < 0)
						B.U[i][j][5] = 1.e-10;
				}
			}
		}
		else if (MP.Nvar == 6) {
			for (int i = 1 + LAP; i <= nx - 1 + LAP; ++i) {
				for (int j = 1 + LAP; j <= ny - 1 + LAP; ++j) {
					if (B.U[i][j][5] < 0)
						B.U[i][j][5] = 1.e-10;
					if (B.U[i][j][6] < 0)
						B.U[i][j][6] = 1.e-10;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------- -
//����LU_SGS������ʱ���ƽ�һ��ʱ�䲽 ����nMesh������ �ĵ�������
void NS_time_advance_LU_SGS(int nMesh)
{
	std::ofstream fcout1, fcout2;
	fcout1.open("time_consuming1.dat", std::ios::app);
	fcout2.open("time_consuming2.dat", std::ios::app);
	clock_t start,mid, finish;
	start = clock();

	Comput_Residual_one_mesh(nMesh);     //���������ϼ���в�

	mid = clock();

	double alfa1 = 0.e0; 
	Mesh_TYPE &	MP = Mesh[nMesh];
	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {

		du_LU_SGS_2D(nMesh, mBlock, alfa1);         //����LU_SGS��������DU = U(n + 1) - U(n)

		finish = clock();

		//double duration = (double)(finish - start) / CLOCKS_PER_SEC;
		double dur1 = (double)(mid - start);
		double dur2 = (double)(finish - mid);
		//printf("time1= %f, time2=%f \n", dur1, dur2);
		fcout1 << dur1 << std::endl;
		fcout2<<"  " << dur2 << std::endl;
		fcout1.close();	//ͳ�����ͨ����ʱ��ͼ���LU_SUS��ʱ��
		fcout2.close();	//ͳ�����ͨ����ʱ��ͼ���LU_SUS��ʱ��

		Block_TYPE & B = MP.Block[mBlock];
		int nx = B.nx; int ny = B.ny;
		//--------------------------------------------------------------------------------------
		//ʱ���ƽ�
		for (int i = 1; i <= nx - 1; ++i) {
			for (int j = 1; j <= ny - 1; ++j) {
				for (int m = 1; m <= Nvar; ++m) {
					B.U[i+LAP][j + LAP][m] +=  B.dU[i][j][m];        //U(n + 1) = U(n) + dU
					//printf("\n B.U[%d][%d][%d] = %10.9e,  B.Res = %10.9e, dU=%10.9e \n", i, j, m, B.U[i + LAP][j + LAP][m], B.Res[i][j][m], B.dU[i][j][m]);
				}
			}
		}
	}
		
	//--------------------------------------------------------------------------------------
	Boundary_condition_onemesh(nMesh);        //�߽����� ���趨Ghost Cell��ֵ��
	update_buffer_onemesh(nMesh);         // ͬ������Ľ�����

	Mesh[nMesh].tt = Mesh[nMesh].tt + dt_global;      //ʱ�� ��ʹ��ȫ��ʱ�䲽����ʱ�����壩
	Mesh[nMesh].Kstep = Mesh[nMesh].Kstep + 1;        //���㲽��
}

//----------------------------------------------------------
//������ճ�Լ�ճ������װ뾶���ڼ��������������ֲ�ʱ�䲽��������ʽ���в��˳�ȣ���ʹ��
void comput_Lvc(int nMesh, int mBlock, flow_var &fl)
{
	double C0, si, sj, s0, ni1, ni2, nj1, nj2, uni, unj, tmp1;
	Block_TYPE & B = Mesh[nMesh].Block[mBlock];                 //��nMesh ������ĵ�mBlock��

	C0 = 1.E0;
	int nx = B.nx; int ny = B.ny;

	//$OMP PARALLEL for( int  DEFAULT(SHARED) PRIVATE(i, j, si, sj, s0, ni1, ni2, nj1, nj2, uni, unj, tmp1)
	for (int i = 1; i <= nx - 1; ++i) {
     	for (int j = 1; j <= ny - 1; ++j) {
			int myi = i + LAP;	int myj = j + LAP;
			s0 = B.vol[i][j];         //���
			si = 0.5E0*(B.si[i][j] + B.si[i + 1][j]);        //������߳�������ƽ����
			ni1 = 0.5E0*(B.ni1[i][j] + B.ni1[i + 1][j]);      //���淨��������ƽ����
			ni2 = 0.5E0*(B.ni2[i][j] + B.ni2[i + 1][j]);
			sj = 0.5E0*(B.sj[i][j] + B.sj[i][j + 1]);
			nj1 = 0.5E0*(B.nj1[i][j] + B.nj1[i][j + 1]);
			nj2 = 0.5E0*(B.nj2[i][j] + B.nj2[i][j + 1]);
			uni = fl.uu[myi][myj] * ni1 + fl.v[myi][myj] * ni2;             //�����ٶ�
			unj = fl.uu[myi][myj] * nj1 + fl.v[myi][myj] * nj2;

			//�װ뾶
			B.Lci[i][j] = (abs(uni) + fl.cc[i][j])*si;         //��ճ��Jocabian������װ뾶 ��Blazek's Book 6.1.4�ڣ�
			B.Lcj[i][j] = (abs(unj) + fl.cc[i][j])*sj;

			tmp1 = gamma / fl.d[myi][myj] * (B.Amu[myi][myj] / Pr + B.Amu_t[myi][myj] / PrT);
			B.Lvi[i][j] = tmp1*si*si / s0;         //ճ����Jocabian�����װ뾶
			B.Lvj[i][j] = tmp1*sj*sj / s0;
		}
	}
	//$OMP END PARALLEL for( int 
}


//���㣨���أ�ʱ�䲽��
void comput_dt(int nMesh, int mBlock)
{
	const double C0 = 1;
	Block_TYPE  & B = Mesh[nMesh].Block[mBlock];                 //��nMesh ������ĵ�mBlock��
	int nx = B.nx; int ny = B.ny;

	//$OMP PARALLEL for( int DEFAULT(SHARED) PRIVATE[i][j]
	for (int i = 1; i <= nx - 1; ++i) {
		for (int j = 1; j <= ny - 1; ++j) {
			if (Iflag_local_dt == 0) {   //ȫ��ʱ�䲽��
				B.dt[i][j] = dt_global;
			}
			else
			{
				//����ʱ�䲽��
				B.dt[i][j] = CFL*B.vol[i][j] / (B.Lci[i][j] + B.Lcj[i][j] + C0*(B.Lvi[i][j] + B.Lvj[i][j]));     //�ֲ�ʱ�䲽��
				if (B.dt[i][j]>dtmax) B.dt[i][j] = dtmax;
				if (B.dt[i][j]<dtmin) B.dt[i][j] = dtmin;
			}
		}
	}
	//$OMP END PARALLEL DO
}

//����1��Euler������ʱ���ƽ�һ��ʱ�䲽 ����nMesh������ �ĵ�������
void NS_Time_advance_1Euler(int nMesh)
{
	Mesh_TYPE & MP = Mesh[nMesh];
	Comput_Residual_one_mesh(nMesh);     //���������ϼ���в�

	if (nMesh != 1) Add_force_function(nMesh);   //���ǿ�Ⱥ�������������Ĵ�����ʹ�ã�

	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {

		Block_TYPE & B = MP.Block[mBlock];
		int nx = B.nx; int ny = B.ny;
		//------------------------------------------------------------------------------
		//ʱ���ƽ�
		//$OMP PARALLEL for(int  DEFAULT(SHARED) PRIVATE(i, j, m, du)
		for (int i = 1; i <= nx - 1; ++i) {
			for (int j = 1; j <= ny - 1; ++j) {
				for (int m = 1; m <= MP.Nvar; ++m) {
					double du = B.Res[i][j][m] / B.vol[i][j];
					B.U[i+LAP][j+LAP][m] += B.dt[i][j] * du;
					//printf("\n\n%d %d \n du= %f,Res= %f,VOL= %f\n dt= %f, U=%f",i,j, du, B.Res[i][j][m], B.vol[i][j], B.dt[i][j], B.U[i + LAP][j + LAP][m]);
				}
			}
		}
		//$OMP do PARALLEL do 
	}

	//-------------------------------------------------------------------------------------- -
	//-------------------------------------------------------------------------------------- -
	Boundary_condition_onemesh(nMesh);         //�߽����� ���趨Ghost Cell��ֵ��
	update_buffer_onemesh(nMesh);        //ͬ������Ľ�����

	Mesh[nMesh].tt = Mesh[nMesh].tt + dt_global;      //ʱ�� ��ʹ��ȫ��ʱ�䲽����ʱ�����壩
	Mesh[nMesh].Kstep = Mesh[nMesh].Kstep + 1;        //���㲽��
													  //print*, "Kstep, t=", Mesh(nMesh) . Kstep, Mesh(nMesh) . tt
	//outputDebug();
}