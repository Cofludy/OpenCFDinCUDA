#include"turbulence_SST.cuh"
#include "common.h"
#include "Global_var.h"
#include "Flow_var.h"
#include "sub_turbulence_SST.h"
#include "cmath"

__device__
double cuda_min(double a, double b) {
	return a < b ? a : b;
}

__device__
double cuda_max(double a, double b) {
	return a > b ? a : b;
}


void Amut_boundary(int mBlock);

__global__
void cuda_calcu_Qt(int *transferInt, double * transferDouble, double * x1, double *y1, double * d, double * uu, double * vv, double *Kt, double * Wt,
	double *dw, double *x, double * y, double *f1, double *Amu, double *Amu_t, double * Res5, double * Res6);

__global__
void cuda_calcu_i_devdiff(int *transferInt, double * transferDouble, double * x, double * y, double *uu, double * vv,
	double *x1, double * y1, double * Kt, double * Wt, double *f1, double * Amu, double * Amu_t, double *Res5, double * Res6);

__global__
void cuda_calcu_j_devdiff(int *transferInt, double * transferDouble, double * x, double * y, double *uu, double * vv,
	double *x1, double * y1, double * Kt, double * Wt, double *f1, double * Amu, double * Amu_t, double *Res5, double *Res6);


void turbulence_SST_kw_before_cuda(double * Amu_t_dev, double * Amu_dev, double *d, double *uu, double *vv, double * T,
	double *U_dev,double *x_dev, double *y_dev, double *x1_dev, double *y1_dev,int mBlock, flow_var & fl, int * transferInt_dev, double * transferDouble_dev)
{
	Block_TYPE & B = Mesh[1].Block[mBlock];
	const int nx =  B.nx;	const int ny =  B.ny;
	const int mm1 = nx + 2 * LAP - 1;	const int nn1 = ny + 2 * LAP - 1;
	const int mm = nx + 2 * LAP;	const int nn = ny + 2 * LAP;

	dim3 threadPerBlock(16, 16);
	dim3  blockPerGrid((nx + 2 * LAP + 1 + threadPerBlock.x - 1) / threadPerBlock.x, (ny + 2 * LAP + 1 + threadPerBlock.y - 1) / threadPerBlock.y);

	//�����������Ϣ�������豸
	double * dw_dev;	double *dw_host;
	HANDLE_ERROR(cudaMallocHost((double **)& dw_host, (nx + 1)*(ny + 1) * sizeof(double)));
	HANDLE_ERROR(cudaMalloc((double **)&dw_dev, (nx + 1)*(ny + 1) * sizeof(double)));
	for (int i = 1; i <= nx; ++i) {
		for (int j = 1; j <= ny; ++j) {
			dw_host[i*(ny + 1) + j] =  B.dw[i][j];
		}
	}
	HANDLE_ERROR(cudaMemcpy(dw_dev, dw_host, (nx + 1)*(ny + 1) * sizeof(double), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaFreeHost(dw_host));


	//������ճ��ϵ������ֵ
	HANDLE_ERROR(cudaMemset(Amu_t_dev, 0, (mm1 + 1)*(nn1 + 1) * sizeof(double)));
	
	/*return*/
	return;


	double * Kt; double *  Wt; double * f1;		//[nx+1][ny+1]
	//double * Qk; double * Qw;		// [mm1 + 1][nn1 + 1]
	
	HANDLE_ERROR(cudaMalloc((double **)& f1, (nx + 1)*(ny + 1) * sizeof(double)));
	HANDLE_ERROR(cudaMemset(f1, 0, (nx + 1)*(ny + 1) * sizeof(double)));	//���㺯��f1 �����ֽ�������Զ������

	//HANDLE_ERROR(cudaMalloc((double **)& Qk, (nx + 1)*(ny + 1) * sizeof(double)));
	//HANDLE_ERROR(cudaMalloc((double **)& Qw, (nx + 1)*(ny + 1) * sizeof(double)));

	HANDLE_ERROR(cudaMalloc((double **)& Kt, (mm1 + 1)*(nn1 + 1) * sizeof(double)));
	HANDLE_ERROR(cudaMalloc((double **)& Wt, (mm1 + 1)*(nn1 + 1) * sizeof(double)));

	double * Res5_dev;	double * Res6_dev;	//[nx+1][ny+1];
	HANDLE_ERROR(cudaMalloc((double **)& Res5_dev, (nx + 1)*(ny + 1) * sizeof(double)));
	HANDLE_ERROR(cudaMalloc((double **)& Res6_dev, (nx + 1)*(ny + 1) * sizeof(double)));

	//����ҳ�����ڴ潫ʣ�������������豸
	double *Kt_host; double * Wt_host;
	HANDLE_ERROR(cudaHostAlloc((double **)&Kt_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));
	HANDLE_ERROR(cudaHostAlloc((double **)&Wt_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));
	for (int i = 1; i <= mm1; ++i) {
		for (int j = 1; j <= nn1; ++j) {
			int flag = i*(nn1 + 1) + j;
			Kt_host[flag] =  B.U[i][j][5] / fl.d[i][j];
			Wt_host[flag] =  B.U[i][j][6] / fl.d[i][j];
		}
	}
	HANDLE_ERROR(cudaMemcpy(Kt, Kt_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaMemcpy(Wt, Wt_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaFreeHost(Kt_host));
	HANDLE_ERROR(cudaFreeHost(Wt_host));

	//��Դ��
	cuda_calcu_Qt<<<blockPerGrid, threadPerBlock>>>(transferInt_dev, transferDouble_dev, x1_dev, y1_dev, d, uu, vv, Kt, Wt, dw_dev,
													x_dev, y_dev, f1, Amu_dev, Amu_t_dev,  Res5_dev, Res6_dev);

	HANDLE_ERROR(cudaThreadSynchronize());
	HANDLE_ERROR(cudaGetLastError());

	//����ɢ��Ͷ�����
	cuda_calcu_i_devdiff << <blockPerGrid, threadPerBlock >> >(transferInt_dev, transferDouble_dev, x_dev, y_dev, uu, vv,
		x1_dev, y1_dev, Kt, Wt, f1, Amu_dev, Amu_t_dev, Res5_dev, Res6_dev);

	cuda_calcu_j_devdiff << <blockPerGrid, threadPerBlock >> >(transferInt_dev, transferDouble_dev, x_dev, y_dev, uu, vv,
		x1_dev, y1_dev, Kt, Wt, f1, Amu_dev, Amu_t_dev, Res5_dev, Res6_dev);

	//���������ݿ�����������
	double *Res5_host;	double *Res6_host;
	HANDLE_ERROR(cudaHostAlloc((double **) & Res5_host, (nx + 1)*(ny + 1) * sizeof(double), cudaHostAllocDefault));
	HANDLE_ERROR(cudaHostAlloc((double **)& Res6_host, (nx + 1)*(ny + 1) * sizeof(double), cudaHostAllocDefault));
	HANDLE_ERROR(cudaMemcpy(Res5_host, Res5_dev, (nx + 1)*(ny + 1) * sizeof(double), cudaMemcpyDeviceToHost));
	HANDLE_ERROR(cudaMemcpy(Res6_host, Res6_dev, (nx + 1)*(ny + 1) * sizeof(double), cudaMemcpyDeviceToHost));

	double *Amu_host;	double *Amu_t_host;
	HANDLE_ERROR(cudaHostAlloc((double **)& Amu_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));
	HANDLE_ERROR(cudaHostAlloc((double **)& Amu_t_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaHostAllocDefault));
	HANDLE_ERROR(cudaMemcpy(Amu_host, Amu_dev, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyDeviceToHost));
	HANDLE_ERROR(cudaMemcpy(Amu_t_host, Amu_t_dev, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyDeviceToHost));
	for (int i = 0; i < (mm1 + 1)*(nn1 + 1); ++i) {
		Amu_host[i] = Amu_host[i] / Re;
		Amu_t_host[i] = Amu_t_host[i] / Re;
	}
	HANDLE_ERROR(cudaMemcpy(Amu_dev, Amu_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaMemcpy(Amu_t_dev, Amu_t_host, (mm1 + 1)*(nn1 + 1) * sizeof(double), cudaMemcpyHostToDevice));

	for (int i = 1; i <= nx - 1; ++i) {
		for (int j = 1; j <= ny - 1; ++j) {
			int flag1 = i*(ny + 1) + j;
			B.Res[i][j][5] = Res5_host[flag1];
			B.Res[i][j][6] = Res6_host[flag1];
			int myi = i + LAP;	int myj = j + LAP;
			int flag2 = myi*(nn1 + 1) + myj;
			B.Amu[myi][myj] = Amu_host[flag2];
			B.Amu_t[myi][myj] = Amu_t_host[flag2];

			/*printf("%d, %d\n", i, j);
			printf("%e, %e, %e, %e\n", B.Res[i][j][5], B.Res[i][j][6], B.Amu[myi][myj], B.Amu_t[myi][myj]);
*/
			/*if (i == 3 && j == 14) {
				printf("T= %e, Amu= %e\n", T[flag], Amu[flag]);
			}*/
		}
	}
	//PAUSE;

	HANDLE_ERROR(cudaFreeHost(Res5_host));
	HANDLE_ERROR(cudaFreeHost(Res6_host));
	HANDLE_ERROR(cudaFreeHost(Amu_host));
	HANDLE_ERROR(cudaFreeHost(Amu_t_host));


	//�趨����ճ��ϵ���������ֵ
	Amut_boundary(mBlock);

	HANDLE_ERROR(cudaFree(f1));
	HANDLE_ERROR(cudaFree(dw_dev));
	//HANDLE_ERROR(cudaFree(Qk));	HANDLE_ERROR(cudaFree(Qw));
	HANDLE_ERROR(cudaFree(Kt)); HANDLE_ERROR(cudaFree(Wt));
	HANDLE_ERROR(cudaFree(Res5_dev)); HANDLE_ERROR(cudaFree(Res6_dev));
}


__global__
void cuda_calcu_Qt(int *transferInt, double * transferDouble, double * x1, double *y1,double * d, double * uu, double * vv,	double *Kt, double * Wt, 
	double *dw, double *x, double * y, double *f1, double *Amu, double *Amu_t,  double * Res5, double * Res6)
{
	__shared__  int nx, ny, LAP, nn1, nn;
	int i = blockDim.x*blockIdx.x + threadIdx.x;
	int j = blockDim.y*blockIdx.y + threadIdx.y;
	nx = transferInt[0];	ny = transferInt[1];
	LAP = transferInt[2];
	nn = ny + 2 * LAP;
	nn1 = ny + 2 * LAP-1;

	double Re = transferDouble[5];

	if (i >= 1 && i <= nx - 1 && j >= 1 && j <= ny - 1) {
		int myi = i + LAP;	int myj = j + LAP;

		//ģ��ϵ��(1Ϊk - wģ�͵�ϵ�����ڽ�����ʹ�ã� 2Ϊk - epslģ�͵�ϵ����Զ����ʹ�ã�
		//SST��һ��k - w��k - epslģ�͵Ļ��ģ�ͣ�ͨ�����غ���f���л�
		const double  beta1_SST = 0.075E0, Cw1_SST = 0.533E0;
		const double  sigma_w2_SST = 0.856E0, beta2_SST = 0.0828E0, Cw2_SST = 0.440E0;
		const double a1_SST = 0.31E0, betas_SST = 0.09E0;

		double Dix, Diy, Djx, Djy, Ds, Dik, Diw, Djk, Djw, Diu, Div, Dju, Djv, kx, ky, Wx, Wy, ux, vx, uy, vy;
		double  omega, Kws, CD_kw, arg1, arg2, arg3, f2, t11, t22, t12, Pk, Pk0;

		int flag = i*(ny + 1) + j;
		int flag1 = (myi + 1)*(nn1 + 1) + myj;
		int flag2 = (myi - 1)*(nn1 + 1) + myj;
		int flag3 = myi*(nn1 + 1) + myj;

		//����������������x, y�ĵ�����ʹ��Jocabian�任
		Dix = ( x1[flag1] -  x1[flag2])*0.5E0;
		Diy = ( y1[flag1] -  y1[flag2])*0.5E0;
		Djx = ( x1[flag3 + 1] -  x1[flag3 - 1])*0.5E0;
		Djy = ( y1[flag3 + 1] -  y1[flag3 - 1])*0.5E0;
		Ds = 1.E0 / (Dix*Djy - Djx*Diy);

		Diu = (uu[flag1] - uu[flag2])*0.5E0;
		Div = (vv[flag1] - vv[flag2])*0.5E0;
		Dik = (Kt[flag1] - Kt[flag2])*0.5E0;
		Diw = (Wt[flag1] - Wt[flag2])*0.5E0;

		Dju = (uu[flag3 + 1] - uu[flag3 - 1])*0.5E0;
		Djv = (vv[flag3 + 1] - vv[flag3 - 1])*0.5E0;
		Djk = (Kt[flag3 + 1] - Kt[flag3 - 1])*0.5E0;
		Djw = (Wt[flag3 + 1] - Wt[flag3 - 1])*0.5E0;

		//����ֵ
		ux = (Diu*Djy - Dju*Diy)*Ds;
		vx = (Div*Djy - Djv*Diy)*Ds;
		kx = (Dik*Djy - Djk*Diy)*Ds;
		Wx = (Diw*Djy - Djw*Diy)*Ds;

		uy = (-Diu*Djx + Dju*Dix)*Ds;
		vy = (-Div*Djx + Djv*Dix)*Ds;
		ky = (-Dik*Djx + Djk*Dix)*Ds;
		Wy = (-Diw*Djx + Djw*Dix)*Ds;

		//��������ճ��ϵ�� �� Blazek's Book Eq. (7.66)    
		//if (i == 3 && j == 14) { printf("AMu= %e  ", Amu[flag3]); }
		Amu[flag3] = Amu[flag3] * Re;
		/*if (i == 3 && j == 14) {
			printf("   %e\n", Amu[flag3]);
		}*/

		omega = vx - uy;      //����
		arg2 = cuda_max(2.E0* sqrt(abs(Kt[flag3])) / (0.09*Wt[flag3] * dw[flag] * Re), 500.E0*Amu[flag3] / 
													(d[flag3] * Wt[flag3] * dw[flag] * dw[flag] * Re*Re));

		f2 = tanh(arg2*arg2);

		//////Revised by Wang XiangYu
		Amu_t[flag3] = cuda_min(cuda_min(d[flag3] * Kt[flag3] / Wt[flag3], a1_SST*d[flag3] * Kt[flag3] * Re / (f2*abs(omega))), 100000.);
		//����f1(ʶ���Ƿ�Ϊ��������������������1��
		Kws = 2.E0*(kx*Wx + ky*Wy)*d[flag3] * sigma_w2_SST / (Wt[flag3] + 1.E-20);      //����������
		CD_kw = cuda_max(Kws, 1.E-20);
		arg3 = cuda_max(sqrt(abs(Kt[flag3])) / (0.09*Wt[flag3] * dw[flag] * Re), 500.E0*Amu[flag3] / (d[flag3] * Wt[flag3] * dw[flag] * dw[flag] * Re*Re));

		arg1 = cuda_min(arg3, 4.E0*d[flag3] * sigma_w2_SST*Kt[flag3] / (CD_kw*dw[flag] * dw[flag]));
		f1[flag] = tanh(arg1*arg1*arg1*arg1);             //���غ�����������������1��Զ����������0  �������л�k - w��k - epsl����)

														  //��Ӧ�� ��ʹ������ճģ�ͣ�
		t11 = ((4.E0 / 3.E0)*ux - (2.E0 / 3.E0)*vy)*Amu_t[flag3] - (2.E0 / 3.E0)*d[flag3] * Kt[flag3] * Re;   //Blazek's Book, Eq. (7.25)
		t22 = ((4.E0 / 3.E0)*vy - (2.E0 / 3.E0)*ux)*Amu_t[flag3] - (2.E0 / 3.E0)*d[flag3] * Kt[flag3] * Re;
		t12 = (uy + vx)*Amu_t[flag3];

		//���ܷ��̵�Դ����� - ��ɢ)
		//Pk = t11*ux + t22*vy + t12*(uy + vx)  //���������� ����Ӧ������Ӧ���ʣ�
		Pk = Amu_t[flag3] * omega*omega;        //��

		Pk0 = cuda_min(Pk, 20.E0*betas_SST*Kt[flag3] * Wt[flag3] * Re*Re);    //������������������ƣ���ֹ���ܹ���

		double Qk = Pk0 / Re - betas_SST*d[flag3] * Wt[flag3] * Kt[flag3] * Re;    //k���̵�Դ��  �������� - ��ɢ�

		double Cw_SST = f1[flag] * Cw1_SST + (1.E0 - f1[flag])*Cw2_SST;    //ģ��ϵ��������f1���������л�
		double beta_SST = f1[flag] * beta1_SST + (1.E0 - f1[flag])*beta2_SST;    //ģ��ϵ��������f1���������л�
		//Qw[flag] = Cw_SST*d[flag]*Pk / ( Amu_t[flag] + 1.d - 20) / Re - beta_SST*d[flag]*Wt[flag]**2 * Re + (1.E0 - f1[flag])*Kws / Re     //W���̵�Դ��
		double Qw = Cw_SST*d[flag3] * omega*omega / Re - beta_SST*d[flag3] * Wt[flag3] * Wt[flag3] * Re + (1.E0 - f1[flag])*Kws / Re;     //W���̵�Դ��

		//����õ�Ԫ����� 
		flag1 = (i + LAP)*(nn + 1) + j + LAP;	flag2 = (i + 1 + LAP)*(nn + 1) + j + LAP;
		const double vol = abs((x[flag1] - x[flag2 + 1])*(y[flag2] - y[flag1 + 1]) -
			(x[flag2] - x[flag1 + 1])*(y[flag1] - y[flag2 + 1]))*0.5e0;
		
		Res5[flag] += Qk * vol;
		Res6[flag] += Qw * vol;
	}
}


//������������ɢ��
__global__
void cuda_calcu_i_devdiff(int *transferInt, double * transferDouble, double * x, double * y, double *uu, double * vv,
	double *x1, double * y1, double * Kt, double * Wt, double *f1, double * Amu, double * Amu_t, double *Res5, double * Res6)
{
	__shared__  int nx, ny, LAP, nn,  nn1;
	int i = blockDim.x*blockIdx.x + threadIdx.x;
	int j = blockDim.y*blockIdx.y + threadIdx.y;
	nx = transferInt[0];	ny = transferInt[1];
	LAP = transferInt[2];
	nn = ny + 2 * LAP;
	nn1 = ny + 2 * LAP - 1;
	double Re = transferDouble[5];

	if (i >= 1 && i <= nx && j >= 1 && j <= ny - 1) {

		const double sigma_k1_SST = 0.85E0, sigma_w1_SST = 0.5E0;
		const double sigma_k2_SST = 1.E0, sigma_w2_SST = 0.856E0;
		double sigma_K_SST, sigma_W_SST;
		double Dix, Diy, Djx, Djy, Ds, Dik, Diw, Djk, Djw, Diu, Div, Dju, Djv, kx, ky, Wx, Wy;

		int myi = i + LAP;	int myj = j + LAP;
		int flagL = (i + LAP)*(nn + 1) + j + 1 + LAP;
		int flagR = (i + LAP)*(nn + 1) + j + LAP;
		double dx = x[flagL] - x[flagR];
		double dy = y[flagL] - y[flagR];
		const double si = sqrt(dx*dx + dy*dy);	//�߳�
		const double ni1 = dy / si;
		const double ni2 = -dx / si;   //normal vector at(i, j) or (I - 1 / 2, J)

		int flag = i*(ny + 1) + j;
		int flag1 = myi*(nn1 + 1) + myj;
		int flag2 = (myi - 1)*(nn1 + 1) + myj;
									   //���������1��ӭ���ʽ
		double un1 = uu[flag2] * ni1 + vv[flag2] * ni2;
		double un2 = uu[flag1] * ni1 + vv[flag1] * ni2;

		//1��L - F ��ʽ
		double Fluxk= -0.5E0*((un1 + abs(un1))*Kt[flag2] + (un2 - abs(un2))*Kt[flag1])*si;
		double Fluxw = -0.5E0*((un1 + abs(un1))*Wt[flag2] + (un2 - abs(un2))*Wt[flag1])*si;

		__syncthreads();

		//ճ�����ɢ�������2�����ĸ�ʽ

		//��ʽϵ����k - w��k - epsl��ʽϵ��֮��ѡ��(f1��Ϊ�л����غ���)
		sigma_K_SST = f1[flag] * sigma_k1_SST + (1.E0 - f1[flag])*sigma_k2_SST;
		sigma_W_SST = f1[flag] * sigma_w1_SST + (1.E0 - f1[flag])*sigma_w2_SST;

		//�����ϵ�ֵ = ����ֵ��ƽ��, �߽��ϵ���ɢϵ�� = �ڲ��ֵ
		double Amu1, Amu2;
		if (i == 1) {
			Amu1 = Amu[flag1] + sigma_K_SST*Amu_t[flag1];         //��ɢϵ��(k����)
			Amu2 = Amu[flag1] + sigma_W_SST*Amu_t[flag1];         //��ɢϵ��(w����)
		}
		else if (i == nx) {
			Amu1 = Amu[flag2] + sigma_K_SST*Amu_t[flag2];        //��ɢϵ��(k����)
			Amu2 = Amu[flag2] + sigma_W_SST*Amu_t[flag2];        //��ɢϵ��(w����)
		}
		else {
			Amu1 = (Amu[flag2] + Amu[flag1] + sigma_K_SST*(Amu_t[flag2] + Amu_t[flag1]))*0.5E0;        //��ɢϵ��(k����), �����ϵ�ֵ = ����ֵ��ƽ��
			Amu2 = (Amu[flag2] + Amu[flag1] + sigma_W_SST*(Amu_t[flag2] + Amu_t[flag1]))*0.5E0;        //��ɢϵ��(w����)
		}

		//������������k, w��������x, y�ĵ��� ������Jocabian�任��
		//----Jocabianϵ�� ����������Լ�������ĵ���, ���ڼ����������ĵ�����
		Dix = x1[flag1] - x1[flag2];
		Diy = y1[flag1] - y1[flag2];
		Djx = (x1[flag2 + 1] + x1[flag1 + 1] - x1[flag2 - 1] - x1[flag1 - 1])*0.25E0;
		Djy = (y1[flag2 + 1] + y1[flag1 + 1] - y1[flag2 - 1] - y1[flag1 - 1])*0.25E0;
		Ds = 1.E0 / (Dix*Djy - Djx*Diy);
		//�������Լ�������ĵ���
		Dik = Kt[flag1] - Kt[flag2];
		Diw = Wt[flag1] - Wt[flag2];
		Djk = (Kt[flag2 + 1] + Kt[flag1 + 1] - Kt[flag2 - 1] - Kt[flag1 - 1])*0.25E0;
		Djw = (Wt[flag2 + 1] + Wt[flag1 + 1] - Wt[flag2 - 1] - Wt[flag1 - 1])*0.25E0;
		//��������x, y����ĵ���
		kx = (Dik*Djy - Djk*Diy)*Ds;
		Wx = (Diw*Djy - Djw*Diy)*Ds;
		ky = (-Dik*Djx + Djk*Dix)*Ds;
		Wy = (-Diw*Djx + Djw*Dix)*Ds;
		//ճ��Ӧ��������ͨ��
		Fluxk += Amu1*(kx*ni1 + ky*ni2)*si / Re;
		Fluxw += Amu2*(Wx*ni1 + Wy*ni2)*si / Re;

		Res5[flag] -= Fluxk;
		Res6[flag] -= Fluxw;

		__syncthreads();
		flag = (i - 1)*(ny + 1) + j;
		Res5[flag] += Fluxk;
		Res6[flag] += Fluxw;
	}
}


//������������ɢ��
__global__
void cuda_calcu_j_devdiff(int *transferInt, double * transferDouble, double * x, double * y, double *uu, double * vv,
	double *x1, double * y1, double * Kt, double * Wt, double *f1, double * Amu, double * Amu_t, double *Res5, double *Res6)
{
	__shared__  int nx, ny, LAP, nn, nn1;
	int i = blockDim.x*blockIdx.x + threadIdx.x;
	int j = blockDim.y*blockIdx.y + threadIdx.y;
	nx = transferInt[0];	ny = transferInt[1];
	LAP = transferInt[2];
	nn = ny + 2 * LAP;
	nn1 = ny + 2 * LAP - 1;
	double Re = transferDouble[5];

	if (i >= 1 && i <= nx-1 && j >= 1 && j <= ny) {

		const double sigma_k1_SST = 0.85E0, sigma_w1_SST = 0.5E0;
		const double sigma_k2_SST = 1.E0, sigma_w2_SST = 0.856E0;
		double sigma_K_SST, sigma_W_SST;
		double Dix, Diy, Djx, Djy, Ds, Dik, Diw, Djk, Djw, Diu, Div, Dju, Djv, kx, ky, Wx, Wy;
	
		//�߳���������
		int flagL = (i + 1 + LAP)*(nn + 1) + j + LAP;
		int flagR = (i + LAP)*(nn + 1) + j + LAP;
		double dx = x[flagL] - x[flagR];
		double dy = y[flagL] - y[flagR];
		const double sj = sqrt(dx*dx + dy*dy);	//�߳�
		const double nj1 = -dy / sj;
		const double nj2 = dx / sj;   //normal vector at(i, j) or (I - 1 / 2, J)

									  //���������1��ӭ���ʽ ��L - F���ѣ�
		int myi = i + LAP;	int myj = j + LAP;
		int flag = i*(ny + 1) + j;
		int flag1 = myi*(nn1 + 1) + myj;
		int flag2 = (myi + 1)*(nn1 + 1) + myj;
		int flag3 = (myi - 1)*(nn1 + 1) + myj;

		double un1 =  uu[flag1 - 1] * nj1 + vv[flag1 - 1] * nj2;
		double un2 =  uu[flag1] * nj1 + vv[flag1] * nj2;
		double Fluxk= -0.5E0*((un1 + abs(un1))*Kt[flag1 - 1] + (un2 - abs(un2))*Kt[flag1])*sj;
		double Fluxw= -0.5E0*((un1 + abs(un1))*Wt[flag1 - 1] + (un2 - abs(un2))*Wt[flag1])*sj;

		__syncthreads();
		//ճ����
		//-------- - Vmyiscous term---------------------------------------------------------------------------- -
		sigma_K_SST = f1[flag] * sigma_k1_SST + (1.E0 - f1[flag])*sigma_k2_SST;
		sigma_W_SST = f1[flag] * sigma_w1_SST + (1.E0 - f1[flag])*sigma_w2_SST;
		double Amu1, Amu2;
		if (j == 1) {
			Amu1 =  Amu[flag1] + sigma_K_SST* Amu_t[flag1];
			Amu2 =  Amu[flag1] + sigma_W_SST* Amu_t[flag1];
		}
		else if (j == ny) {
			Amu1 =  Amu[flag1 - 1] + sigma_K_SST* Amu_t[flag1 - 1];
			Amu2 =  Amu[flag1 - 1] + sigma_W_SST* Amu_t[flag1 - 1];
		}
		else {
			Amu1 = ( Amu[flag1] +  Amu[flag1 - 1] + sigma_K_SST*( Amu_t[flag1] +  Amu_t[flag1 - 1]))*0.5E0;
			Amu2 = ( Amu[flag1] +  Amu[flag1 - 1] + sigma_W_SST*( Amu_t[flag1] +  Amu_t[flag1 - 1]))*0.5E0;
		}

		//������������k, w��������x, y�ĵ��� ������myjocabmyian�任��
		Dix = ( x1[flag2 - 1] +  x1[flag2] -  x1[flag3 - 1] -  x1[flag3])*0.25E0;
		Diy = ( y1[flag2 - 1] +  y1[flag2] -  y1[flag3 - 1] -  y1[flag3])*0.25E0;
		Djx =  x1[flag1] -  x1[flag1 - 1];
		Djy =  y1[flag1] -  y1[flag1 - 1];
		Ds = 1.E0 / (Dix*Djy - Djx*Diy);

		Dik = (Kt[flag2 - 1] + Kt[flag2] - Kt[flag3 - 1] - Kt[flag3])*0.25E0;
		Diw = (Wt[flag2 - 1] + Wt[flag2] - Wt[flag3 - 1] - Wt[flag3])*0.25E0;
		Djk = Kt[flag1] - Kt[flag1 - 1];
		Djw = Wt[flag1] - Wt[flag1 - 1];
		//
		kx = (Dik*Djy - Djk*Diy)*Ds;
		Wx = (Diw*Djy - Djw*Diy)*Ds;
		ky = (-Dik*Djx + Djk*Dix)*Ds;
		Wy = (-Diw*Djx + Djw*Dix)*Ds;

		Fluxk += Amu1*(kx*nj1 + ky*nj2)*sj / Re;
		Fluxw += Amu2*(Wx*nj1 + Wy*nj2)*sj / Re;
		__syncthreads();
	
		Res5[flag] -= Fluxk;
		Res6[flag] -= Fluxw;

		__syncthreads();
		flag = i*(ny + 1) + j - 1;
		Res5[flag] += Fluxk;
		Res6[flag] += Fluxw;
	}
}


//ճ��ϵ���������ϵ�ֵ ���̱ڱ߽���÷�ֵ���Ա�֤�̱��ϵ�ƽ������ճ��ϵ��Ϊ0��
void Amut_boundary(int mBlock)
{
	int ib, ie, jb, je;
	Block_TYPE & B = Mesh[1].Block[mBlock];

	int nx = B.nx; int ny = B.ny;

	//Ghost Cell ��� mutֵΪ �ڵ�mutֵ*�� - 1��(��������ʹ������mut = 0)

	for (int ksub = 1; ksub <= B.subface; ++ksub) {
		BC_MSG_TYPE & Bc = B.bc_msg[ksub];
		if (Bc.neighb == BC_Wall) {   //(ճ��)����߽�����
			ib = Bc.ist; ie = Bc.iend;
			jb = Bc.jst; je = Bc.jend;

			if (Bc.face == 1) {   //i -
				for (int j = jb; j <= je - 1; ++j) {
					B.Amu_t[0 + LAP][j + LAP] = -B.Amu_t[1 + LAP][j + LAP];
				}
			}
			else if (Bc.face == 3) {   //i +
				for (int j = jb; j <= je - 1; ++j) {
					B.Amu_t[nx + LAP][j + LAP] = -B.Amu_t[nx - 1 + LAP][j + LAP];      //mut
				}
			}
			else if (Bc.face == 2) {   //j -
				for (int i = ib; i <= ie - 1; ++i) {
					B.Amu_t[i + LAP][0 + LAP] = -B.Amu_t[i + LAP][1 + LAP];       //mut
				}
			}
			else if (Bc.face == 4) {   //j +
				for (int i = ib; i <= ie - 1; ++i) {
					B.Amu_t[i + LAP][ny + LAP] = -B.Amu_t[i + LAP][ny - 1 + LAP];       //mut
				}
			}
		}
	}
}

