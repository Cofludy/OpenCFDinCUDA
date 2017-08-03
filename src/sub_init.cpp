//��ʼ���������������ݽṹ�͸���ֵ
//���ڶ������񣬴�����������

#include<stdlib.h>
#include"sub_init.h"
#include"Global_var.h"
#include"common.h"
#include "sub_boundary.h"
#include "sub_NS_singlegid.h"
#include "sub_NS_multigrid.h" 
#include "main.h"


#include<fstream>
#include<string>
#include<iostream>
using namespace std;


void init_flow_zero();
void init_flow_read();

extern int * transferInt_dev = nullptr;
extern double * transferDouble_dev = nullptr;
extern double * x1_dev = nullptr;
extern double * y1_dev = nullptr;
extern double * x_dev = nullptr;
extern double * y_dev = nullptr;


//��������Ƿ������ڶ�������
//������������ = 2 * K + 1 ����2������ = 4 * K + 1 ����3������ = 8 * K + 1 ����4������ ...
void check_mesh_multigrid()
{
	printf("Read Mesh2d.dat, Check if Multi-Grid can be used ...\n");
	std::ifstream fcin;
	fcin.open("Mesh2d.dat");
	int NB;
	fcin >> NB;
	int * NI = (int *)malloc((NB + 1) * sizeof(int));
	int * NJ = (int *)malloc((NB + 1) * sizeof(int));
	NI[0] = 0;	NJ[0] = 0;
	for (int i = 1; i <= NB; ++i) {
		fcin >> NI[i] >> NJ[i];
	}
	fcin.close();

	int N_Cell = 0;
	int Km_grid = NI[1];  //��ʼֵ
	int Km = 1;	//���������������
	for (int m = 1; m <= NB; ++m) {
		N_Cell = N_Cell + (NI[m] - 1)*(NJ[m] - 1);  //ͳ��������Ԫ��

		//�жϿ�ʹ�õ���������
		Km = 1;	
		int NN = 2;
		//�ж�׼�� ������ - 1 �ܱ�2**km ������ ����ϡ������Ԫ����С��2
		for (; 0 == (NI[m] - 1) % NN && (NI[m] - 1) / NN >= 2 &&
			0 == (NJ[m] - 1) % NN && (NJ[m] - 1) / NN >= 2; Km++) {
			NN = NN * 2;
		}
		Km_grid = Km_grid < Km ? Km_grid : Km;
	}
	printf("Finished check Mesh2d.dat,  Most stage is %d \n", Km_grid);
	printf(" Check bc2d.in ...\n");

	fcin.open("bc2d_1.in");
	std::string tempStr;
	getline(fcin, tempStr);
	getline(fcin, tempStr);
	int Ntmp;
	fcin >> Ntmp;
	
	int Bsub = 0;
	int f_no, face, ist, iend, jst, jend, neighb, subface, orient;
	for (int m = 1; m <= NB; ++m) {
		getline(fcin, tempStr);
		getline(fcin, tempStr);
		fcin >> Bsub;	//number of the subface in the Block m
		getline(fcin, tempStr);
		for (int ksub = 1; ksub <= Bsub; ++ksub) {
			fcin >> f_no >> face >> ist >> iend >> jst >> jend >> neighb >> subface >> orient;
			int NN = 1;
			int Km = 1;
			for (; 0 == (ist - 1) % NN && 0 == (iend - 1) % NN &&
				0 == (jst - 1) % NN && 0 == (jend - 1) % NN; Km++) {
				NN = NN * 2;
			}
			Km_grid = Km_grid < Km ? Km_grid : Km;
		}
	}
	fcin.close();

	printf("Check multigrid OK\n");
	printf("Total Block number is %d, Total Cell number is %d \n", NB, N_Cell);
	printf("Most stage number of multi-grid is %d\n", Km_grid);
	printf("------------------------------\n");

	Num_Mesh = min(3, Km_grid, Num_Mesh);		//�趨�������������汾�������3������
	printf(" %d   stage grids is used !\n", Num_Mesh);

	free(NI);
	free(NJ);
}

void Init()
{
	//initial of const variables
	Cv = 1.E0 / (gamma*(gamma - 1.E0)*Ma*Ma);
	//�����ݽṹ�������硱�����Ա������飩
	Mesh = (Mesh_TYPE *)malloc((Num_Mesh+1) * sizeof(Mesh_TYPE));

	Creat_Mesh1();	//������ϸ������ (�������ļ�Mesh2d.dat)
	read_bcin();	//������������Ϣ (bc2d.in)
	if (Num_Mesh >= 2) {
		Creat_Mesh(1, 2);    //����1��������ϸ������Ϣ������2�����񣨴�����
	}
	if (Num_Mesh >= 3) {
		Creat_Mesh(2, 3);    //����2��������Ϣ�������񣩣� ����3�������������
	}

}

// �������ݽṹ�� ��ϸ���� �����漸�������غ������
void Creat_Mesh1()
{
	printf("_________________________\n");
	printf("read Mesh2d.dat\n");

	std::ifstream fcin;
	fcin.open("Mesh2d.dat");
	int NB;	//block number
	fcin >> NB;
	Mesh[1].Num_Block = NB;
	Mesh[1].Num_Cell = 0;
	Mesh[1].Block = (Block_TYPE *)malloc((NB + 1) * sizeof(Block_TYPE));
	int * NI = (int *)malloc((NB + 1) * sizeof(int));
	int * NJ = (int *)malloc((NB + 1) * sizeof(int));
	for (int i = 1; i <= NB; ++i) {
		fcin >> NI[i] >> NJ[i];
	}

	for (int m = 1; m <= NB; ++m) {
		Block_TYPE &B =  Mesh[1].Block[m];
		B.Block_no = m;
		B.FVM_FDM = Method_FVM;   //��ֵ������Ĭ��Ϊ���������
		B.nx = NI[m]; B.ny = NJ[m];
		int nx = B.nx;	int ny = B.ny;
		Mesh[1].Num_Cell = Mesh[1].Num_Cell + (nx - 1)*(ny - 1);

		//�����ڴ�(x, y) �ڵ����ꣻ(x1, y1)������������; s0 ����������� U, Un �غ����

		//------���°�������������Fortran�о����±�� 1-LAP ��ʼ------------
		int mm = nx + 2 * LAP;	int nn = ny + 2 * LAP;
		allocMatrix( (B.x), mm, nn);
		allocMatrix( (B.y), mm, nn);
		int mm1 = nx + 2 * LAP-1;	int nn1 = ny + 2 * LAP-1;
		allocMatrix( (B.x1), mm1, nn1);
		allocMatrix( (B.y1), mm1, nn1);
		//�غ����(4�������������6��������4��������� + k + w)
		allocMatrix(B.U, mm1, nn1, Nvar);
		//����֮��ɴ������ֵ����, ��������ʹ�ã�4��������k, w���̲�����
		allocMatrix(B.deltU, mm1, nn1, 4);

		allocMatrix(B.Amu, mm1, nn1);
		allocMatrix(B.Amu_t, mm1, nn1);
		//-------------------------------------------------------

		allocMatrix(B.Res, nx - 1, ny - 1, Nvar);		//�в�
		allocMatrix(B.dt, mm1, nn1);	//ʱ�䲽��

		allocMatrix(B.dU, nx, ny, Nvar);	//= U(n + 1) - U(n), LU - SGS��ʹ��

		//������
		allocMatrix(B.vol, nx, ny);
		allocMatrix(B.si, nx, ny); allocMatrix(B.sj, nx, ny); 
		allocMatrix(B.ni1, nx, ny); allocMatrix(B.ni2, nx, ny);
		allocMatrix(B.nj1, nx, ny); allocMatrix(B.nj2, nx, ny);
		//�װ뾶
		allocMatrix(B.Lci, nx, ny); allocMatrix(B.Lcj, nx, ny);
		allocMatrix(B.Lvi, nx, ny); allocMatrix(B.Lvj, nx, ny);
	
		allocMatrix(B.Un, nx, ny, Nvar);	//��һʱ�䲽��ֵ

		if (Time_Method == Time_Dual_LU_SGS) {
			allocMatrix(B.Un1, nx, ny);		//n-1ʱ�䲽��ֵ�� ˫ʱ�䲽LU_SGS�����в���
		}

		//�����涯̬������ڴ��ַ���г�ʼ������
		//����ʡ��....

		for (int i = 0; i <= mm1; ++i) {
			for (int j = 0; j <= nn1; ++j) {
				B.x1[i][j] = 0;	B.y1[i][j] = 0;
				B.U[i][j][1] = 1.0; B.U[i][j][2] = 0.0;
				B.U[i][j][3] = 0.0; B.U[i][j][4] = 1.0;

				B.Amu[i][j] = 0;	B.Amu_t[i][j] = 0;
				for (int k = 0; k <= 4; ++k) {
					B.deltU[i][j][k] = 0.0;
				}
				B.dt[i][j] = 0.0;
			}
		}
		for (int i = 0; i <= nx; ++i) {
			for (int j = 0; j <= ny; ++j) {
				B.vol[i][j] = 0;
				B.Un[i][j][1] = 1.0; B.Un[i][j][2] = 0.0;
				B.Un[i][j][3] = 0.0; B.Un[i][j][4] = 0.0;
				B.Lci[i][j] = 0; B.Lcj[i][j] = 0;
				B.Lvi[i][j] = 0; B.Lvj[i][j] = 0;
				for (int k = 0; k <= Nvar; ++k) {
					B.dU[i][j][k] = 0;
				}

			}
		}

		if (Nvar == 5) {
			for (int i = 0; i <= mm1; ++i) {
				for (int j = 0; j <= nn1; ++j) {
					B.U[i][j][5] = 1.0 / Re;
				}
			}
		}
		else if(Nvar ==6 ) {
			for (int i = 0; i <= mm1; ++i) {
				for (int j = 0; j <= nn1; ++j) {
					B.U[i][j][5] = 0;		//�Ķ���
					B.U[i][j][6] = 1.0 / Re;	//���ܱȺ�ɢ��
				}
			}
		}
		for (int j = 1; j <= ny; ++j) {
			for (int i = 1; i <= nx; ++i) {
				fcin >> B.x[i+LAP][j+LAP];
			}
		}
		for (int j = 1; j <= ny; ++j) {
			for (int i = 1; i <= nx; ++i) {
				fcin >> B.y[i+LAP][j+LAP];
			}
		}

		//�������������
		//����������������Ҫ�Ȼ��Ghost Cell����Ϣ�����
		for (int i = 1; i <= nx - 1; ++i) {
			for (int j = 1; j <= ny - 1; ++j) {
				//����������������
				B.vol[i][j] = abs((B.x[i+LAP][j + LAP] - B.x[i + 1 + LAP][j + 1 + LAP])*(B.y[i + 1 + LAP][j + LAP] - B.y[i + LAP][j + 1 + LAP]) -
					(B.x[i + 1 + LAP][j + LAP] - B.x[i + LAP][j + 1 + LAP])*(B.y[i + LAP][j + LAP] - B.y[i + 1 + LAP][j + 1 + LAP]))*0.5e0;
			}
		}
		//printf("vol[][]= %f, %f \n", B.vol[nx-1][ny-1], B.vol[nx-5][ny-4]);	PAUSE;

		//������ ���߳���������
		for (int i = 1; i <= nx; ++i) {
			for (int j = 1; j <= ny - 1; ++j) {
				double dx = B.x[i + LAP][j + 1 + LAP] - B.x[i + LAP][j + LAP];
				double dy = B.y[i + LAP][j + 1 + LAP] - B.y[i + LAP][j + LAP];
				B.si[i][j] = sqrt(dx*dx + dy*dy);	//�߳�
				B.ni1[i][j] = dy / B.si[i][j];
				B.ni2[i][j] = -dx / B.si[i][j];   //normal vector at(i, j) or (I - 1 / 2, J)
			}
		}
		for (int i = 1; i <= nx-1; ++i) {
			for (int j = 1; j <= ny; ++j) {
				double dx = B.x[i + 1 + LAP][j + LAP] - B.x[i + LAP][j + LAP];
				double dy = B.y[i + 1 + LAP][j + LAP] - B.y[i + LAP][j + LAP];
				B.sj[i][j] = sqrt(dx*dx + dy*dy);     //length
				B.nj1[i][j] = -dy / B.sj[i][j];
				B.nj2[i][j] = dx / B.sj[i][j];     //normal vector at i, j + 1 / 2
			}
		}
	} 
	fcin.close();
	free(NI);	free(NJ);

	//ʱ�䲽��ʱ��
	Mesh[1].Kstep = 0;
	Mesh[1].tt = 0.0;
	printf("read Mesh2d.dat OK\n");
}

//----Mesh control message(bc2d.in)------------------------------------------
void read_bcin()
{
	printf("read bc2d.in ......\n");
	std::ifstream fcin;
	fcin.open("bc2d_1.in");
	std::string tempStr;
	getline(fcin, tempStr);
	getline(fcin, tempStr);
	int NB;
	fcin >> NB;
	if (NB != Mesh[1].Num_Block) {
		printf("Error!  Block number in bc2d.in is not equal to that in Mesh2d.dat !\n");
		PAUSE;
		exit(1);
	}
	for (int m = 1; m <= NB; ++m) {
		Block_TYPE &B = Mesh[1].Block[m];
		getline(fcin, tempStr);
		getline(fcin, tempStr);
		getline(fcin, tempStr);
		fcin >> B.subface;		//number of the subface in the Block m
		getline(fcin, tempStr);
		getline(fcin, tempStr);
		B.bc_msg = (BC_MSG_TYPE *)malloc((B.subface + 1) * sizeof(BC_MSG_TYPE));
		for (int ksub = 1; ksub <= B.subface; ++ksub) {
			BC_MSG_TYPE & Bc = B.bc_msg[ksub];
			fcin >> Bc.f_no >> Bc.face >> Bc.ist >> Bc.iend >> Bc.jst >> Bc.jend >> Bc.neighb >> Bc.subface >> Bc.orient;
			//cout<< Bc.f_no << Bc.face << Bc.ist << Bc.iend << Bc.jst << Bc.jend << Bc.neighb << Bc.subface << Bc.orient;
		}
	}
	fcin.close();
	printf("read bc2d.in OK");
}


//---------------------------------------------------------------------------------- -
//�����ϼ�������Ϣ������������m2(ϡ������)
//ϡ������ʹ������ģ�ͣ�������Ϊ4
void Creat_Mesh(int m1, int  m2)
{
	printf("Creat_Mesh this creat mesh is undefined!\n");

	PAUSE;
}

//��ʼ������
void Init_flow()
{
	if (Iflag_init == 0) {
		init_flow_zero();		//����������������������ٲ�ֵ��ϸ����
	}
	else {
		init_flow_read();		//���ļ���ȡ����
	}

	if (Time_Method == Time_Dual_LU_SGS) {
		Mesh_TYPE & MP = Mesh[1];
		for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
			Block_TYPE & B = MP.Block[mBlock];
			int nx = B.nx;	int ny = B.ny;
			for (int i = 1; i <= nx - 1; ++i) {
				for (int j = 1; j <= ny - 1; ++j) {
					for (int m = 1; m <= Nvar; ++m) {
						B.Un[i][j][m] = B.U[i + LAP][j + LAP][m];
						B.Un1[i][j][m] = B.U[i + LAP][j + LAP][m];
					}
				}
			}
		}
	}

	if (USEGPU) {
		Block_TYPE B = Mesh[1].Block[1];
		int nx = B.nx; int ny = B.ny;
		int mm1 = nx + 2 * LAP - 1;
		int nn1 = ny + 2 * LAP - 1;
		double 	p00 = 1.e0 / (gamma*Ma*Ma);

		//��һЩ��ɢ�ĳ���������ݵ�GPU��
		int transferInt[5] = { nx, ny, LAP, Nvar };
		//					   0   1   2     3
		double transferDouble[15] = { p00, gamma, Cv , Ma, T_inf, Re, Cp, Pr, PrT };
		//                             0     1     2    3    4     5   6   7   8
		//int * transferInt_dev;		double * transferDouble_dev;
		HANDLE_ERROR(cudaMalloc((int **)& transferInt_dev, 15 * sizeof(int)));
		HANDLE_ERROR(cudaMalloc((double **)& transferDouble_dev, 15 * sizeof(double)));
		HANDLE_ERROR(cudaMemcpy(transferInt_dev, transferInt, 15 * sizeof(int), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaMemcpy(transferDouble_dev, transferDouble, 15 * sizeof(double), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaDeviceSynchronize());

		//��������Ϣ��ŵ�GPUȫ���ڴ���

		//������������ ���ݵ�GPU��,����ʹ��ҳ�����ڴ������ݲ���
		/*	double * x1_dev;	double * y1_dev;*/
		double *x1_host;	double *y1_host;	//�����ϵ�ҳ�����ڴ�
		HANDLE_ERROR(cudaHostAlloc((double **)& x1_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));
		HANDLE_ERROR(cudaHostAlloc((double **)& y1_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));

		HANDLE_ERROR(cudaMalloc((double **)& x1_dev, (mm1 + 1)*(nn1 + 1) * sizeof(double)));
		HANDLE_ERROR(cudaMalloc((double **)& y1_dev, (mm1 + 1)*(nn1 + 1) * sizeof(double)));

		for (int i = 0; i <= mm1; ++i) {
			for (int j = 0; j <= nn1; ++j) {
				//printf("%d, %d\n", i, j);
				x1_host[i*(nn1 + 1) + j] = B.x1[i][j];
				y1_host[i*(nn1 + 1) + j] = B.y1[i][j];
			}
		}
		HANDLE_ERROR(cudaMemcpy(x1_dev, x1_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaMemcpy(y1_dev, y1_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaFreeHost(x1_host));	x1_host = nullptr;
		HANDLE_ERROR(cudaFreeHost(y1_host));	y1_host = nullptr;

		//�������� ���ݵ�GPU�ϣ� ����ʹ��ҳ�����ڴ����
		//double * x_dev;	double * y_dev;
		double *x_host;	double *y_host;	//�����ϵ�ҳ�����ڴ�
		int mm = nx + 2 * LAP;	int nn = ny + 2 * LAP;
		HANDLE_ERROR(cudaHostAlloc((double **)& x_host, (mm + 1)*(nn + 1) * sizeof(double), cudaHostAllocDefault));
		HANDLE_ERROR(cudaHostAlloc((double **)& y_host, (mm + 1)*(nn + 1) * sizeof(double), cudaHostAllocDefault));

		HANDLE_ERROR(cudaMalloc((double **)& x_dev, (mm + 1)*(nn + 1) * sizeof(double)));
		HANDLE_ERROR(cudaMalloc((double **)& y_dev, (mm + 1)*(nn + 1) * sizeof(double)));

		for (int i = 0; i <= mm; ++i) {
			for (int j = 0; j <= nn; ++j) {
				//printf("%d, %d\n", i, j);
				x_host[i*(nn + 1) + j] = B.x[i][j];
				y_host[i*(nn + 1) + j] = B.y[i][j];
			}
		}
		HANDLE_ERROR(cudaMemcpy(x_dev, x_host, (mm + 1)*(nn + 1) * sizeof(double), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaMemcpy(y_dev, y_host, (mm + 1)*(nn + 1) * sizeof(double), cudaMemcpyHostToDevice));
		HANDLE_ERROR(cudaFreeHost(x_host));	x_host = nullptr;
		HANDLE_ERROR(cudaFreeHost(y_host));	y_host = nullptr;

	}

	printf("Initiallize OK ... ... \n");
}

//��������ʼ���� ������������£����������ʼ���㣨Ȼ���ֵ��ϸ����
void init_flow_zero()
{
	double d0 = 1.0; double u0 = 1.0*cos(AoA); 
	double v0 = 1.e0*sin(AoA); double p0 = 1.0 / (gamma*Ma*Ma);

	if (Nvar == 5) {
		Mesh_TYPE &MP = Mesh[1];	//������
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE & B = MP.Block[m];
			for (int i = 1+LAP; i <= B.nx - 1 + LAP; ++i) {
				for (int j = 1 + LAP; j <= B.ny - 1 + LAP; ++j) {
					B.U[i][j][5] = vt_inf / Re;		//Vtֵ ��ͨ���趨Ϊ����ճ��ϵ����3-5����
				}
			}
		}
	}

	// k��w�ĳ�ֵ ��0��ֵ������������������ֵ��
	if (Nvar == 6) {
		Mesh_TYPE &MP = Mesh[1];	//������
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE & B = MP.Block[m];
			for (int i = 1 + LAP; i <= B.nx - 1 + LAP; ++i) {
				for (int j = 1 + LAP; j <= B.ny - 1 + LAP; ++j) {
					B.U[i][j][5] = 10.e0*Kt_inf;		//Vtֵ ��ͨ���趨Ϊ����ճ��ϵ����3-5����
					B.U[i][j][6] = Wt_inf;		// ���ܱȺ�ɢ�ʣ���ֵΪ����ֵ 
				}
			}
		}
	}

	//-------------------------------------------------------------------- -
	//�������ĳ�ֵ���ɴ������𼶼����ֵ����
	Mesh_TYPE &MP = Mesh[Num_Mesh];        //��ϡ�������
	for (int m = 1; m <= MP.Num_Block; ++m) {
		Block_TYPE & B = MP.Block[m];	//Mesh(Num_Mesh) ����ֵ�����
		for (int i = 1 + LAP; i <= B.nx - 1 + LAP; ++i) {
			for (int j = 1 + LAP; j <= B.ny - 1 + LAP; ++j) {
				B.U[i][j][1] = d0;
				B.U[i][j][2] = d0*u0;
				B.U[i][j][3] = d0*v0;
				B.U[i][j][4] = p0 / (gamma - 1.0) + 0.50*d0*(u0*u0 + v0*v0);
			}
		}
	}

	Boundary_condition_onemesh(Num_Mesh);            //�߽����� ���趨Ghost Cell��ֵ��
	update_buffer_onemesh(Num_Mesh);           //ͬ������Ľ�����
	
	//---------------------------------------------------------------- -
	//------------------------------------------------------
	//׼����ֵ�Ĺ���
	//�����������㣬�𼶲�ֵ��ϸ����
	
	for (int nMesh = Num_Mesh; nMesh >= 1; --nMesh) {
		for (int step = 1; step <= Pre_Step_Mesh[nMesh]; ++step) {
			NS_Time_advance(nMesh);			
			if (step % Kstep_show == 0) {
				output_Res(nMesh);		//�������չʾ���
			}
		}

		if (nMesh > 1) {
			prolong_U(nMesh, nMesh - 1, 1);                //��nMesh�������ϵ���������ֵ����һ������; flag = 1 ��ֵU����
			Boundary_condition_onemesh(nMesh - 1);         //�߽����� ���趨Ghost Cell��ֵ��
			update_buffer_onemesh(nMesh - 1);              //ͬ������Ľ�����
			printf(" Prolong  to mesh %d OK!\n", nMesh - 1);
		}
	}
}

//��flow2d.dat �ļ���ȡ��ֵ �����ܵ����񣩣� ����Ϊtecplot��ʽ
void init_flow_read()
{
	double d0, u0, v0, p0, T0, kt0, kw0, tmp;

	Mesh_TYPE & MP = Mesh[1];             //���ܵ�����
	printf("Init from 'flow2d.dat' ......");
	ifstream fcin;
	fcin.open("flow2d.dat");
	string tempStr;
	getline(fcin, tempStr);
	for (int m = 1; m <= MP.Num_Block; ++m) {
		Block_TYPE & B = MP.Block[m];                 //�����
		getline(fcin, tempStr);
		for (int j = LAP; j <= B.ny+LAP; ++j) {
			for (int i = LAP; i <= B.nx+LAP; ++i) {
				fcin >> tmp >> tmp >> d0 >> u0 >> v0 >> T0>>tmp>>tmp;
				B.U[i][j][1] = d0;                    //�ܶ�
				B.U[i][j][2] = d0*u0;                 //x - �������ܶ�
				B.U[i][j][3] = d0*v0;                 //y - �������ܶ�
				B.U[i][j][4] = d0*Cv*T0 + 0.5E0*d0*(u0*u0 + v0*v0);  //�������ܶ�
			}
		}
	}
	fcin.close();
	if (MP.Nvar == 5) {
		fcin.open("SA2d.dat");	//��ȡvt
		getline(fcin, tempStr);
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE &  B = MP.Block[m];
			getline(fcin, tempStr);
			for (int j = 0; j <= B.ny; ++j) {
				for (int i = 0; i <= B.nx; ++i) {
					fcin >> tmp >> tmp >> B.U[i][j][5];
				}
			}
		}
	}
	fcin.close();

	if (MP.Nvar == 5) {
		fcin.open("SST2d.dat"); //��ȡKt, Wt
		getline(fcin, tempStr);
		for (int m = 1; m <= MP.Num_Block; ++m) {
			Block_TYPE &  B = MP.Block[m];
			getline(fcin, tempStr);
			for (int j = 0; j <= B.ny; ++j) {
				for (int i = 0; i <= B.nx; ++i) {
					fcin >> tmp >> tmp >> B.U[i][j][6] >> B.U[i][j][6];
				}
			}
		}
		fcin.close();
	}

	Boundary_condition_onemesh(1);                   //����߽�����
	update_buffer_onemesh(1);                        //�ڱ߽�����
}