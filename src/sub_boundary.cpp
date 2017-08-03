
#include"Global_var.h"
#include"sub_boundary.h"
#include"common.h"
#include<string>
#include<fstream>


void Symmetry_or_slidewall(int nMesh, int mBlock, int ksub);
void boundary_Farfield_inlet_outlet(int nMesh, int mBlock, int ksub, int Bc_neighb);
void boundary_wall(int nMesh, int mBlock, int ksub);
void boundary_move_wall(int nMesh, int mBlock, int ksub);
void U_average_conner(int Nvar1, double U1[], double U2[], double U3[], double*  U4, double Cv);

//---------------------------------------------------------------------------- -
//�������ӹ�ϵ������Ӧ�������д�뻺���������������ĵ�����
void Update_coordinate_buffer()
{
	for (int nMesh = 1; nMesh <= Num_Mesh; ++nMesh) {
		Update_coordinate_buffer_onemesh(nMesh);
	}
}

//-------- - Continue boundary(inner boundary)------------------------------ -
//�������ӹ�ϵ������Ӧ�������д�뻺���������������ĵ�����(����1������)
//�������񽻽������� LAP��������
//����������ļ�����Ϣ
//��������߽磬�������ķ�������������ļ�����Ϣ�����ø߽���壩

void Update_coordinate_buffer_onemesh(int nMesh)
{
	Mesh_TYPE &MP = Mesh[nMesh];	//���� ��nMesh=3,2,1���� �֡��С�������

	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
		Block_TYPE & B = MP.Block[mBlock];
		for (int ksub = 1; ksub <= B.subface; ++ksub) {
			BC_MSG_TYPE & Bc = B.bc_msg[ksub];
			int ibegin2 = Bc.ist; int iend2 = Bc.iend; 
			int jbegin2 = Bc.jst; int jend2 = Bc.jend;     //write

			if (Bc.neighb > 0) {	//inner boundary
				int n1 = Bc.iend - Bc.ist + Bc.jend - Bc.jst + 1;  //i_end - i_begin + j_end - j_begin + 1
				double *** Ux;
				allocMatrix(Ux, LAP, n1, 2);
				int m_neighbour = Bc.neighb;  int msub = Bc.subface;
				Block_TYPE &B1 = Mesh[nMesh].Block[m_neighbour];
				BC_MSG_TYPE &Bc1 = B1.bc_msg[msub];
				int ibegin1 = Bc1.ist; int iend1 = Bc1.iend; 
				int jbegin1 = Bc1.jst; int jend1 = Bc1.jend;   //read
				

				for (int i = 1; i <= LAP; ++i) {
					if (Bc1.face == 1) {	//blundary i-
						for (int j = jbegin1; j <= jend1; ++j) {
							Ux[i][j][1] = B1.x[ibegin1 + i + LAP][j + LAP];
							Ux[i][j][2] = B1.y[ibegin1 + i + LAP][j + LAP];
						}
					}
					else if (Bc1.face == 2) {	//boundary j-
						for (int j = ibegin1; j <= iend1; ++j) {
							Ux[i][j][1] = B1.x[j + LAP][jbegin1 + i + LAP];
							Ux[i][j][2] = B1.y[j + LAP][jbegin1 + i + LAP];
						}
					}
					else if (Bc1.face == 3) {	//boundary i+
						for (int j = jbegin1; j <= jend1; ++j) {
							Ux[i][j][1] = B1.x[iend1 - 1 + i][j + LAP];
							Ux[i][j][2] = B1.y[iend1 - 1 + i][j + LAP];
						}
					}
					else {			// boundary j+
						for (int j = ibegin1; j <= iend1; ++j) {
							Ux[i][j][1] = B1.x[j + LAP][jend1 - 1 + i];
							Ux[i][j][2] = B1.y[j + LAP][jend1 - 1 + i];
						}
					}
				}

				int orient = Bc.orient;
				int k1 = 1;
				for (int k = 1; k <= n1; ++k) {
					if (orient == 2) {
						k1 = k;
					}
					else {
						k1 = n1 + 1 - k;
					}
					for (int i = 1; i <= LAP; ++i) {
						if (Bc.face == 1) {	//blundary i-
							B.x[ibegin2 - 1 + i][jbegin2 + k - 1 + LAP] = Ux[i][k1][1];
							B.y[ibegin2 - 1 + i][jbegin2 + k - 1 + LAP] = Ux[i][k1][2];
						}
						else if (Bc.face == 2) {	//boundary j-
							B.x[ibegin2 + k - 1 + LAP][jbegin2 - 1 + i] = Ux[i][k1][1];
							B.y[ibegin2 + k - 1 + LAP][jbegin2 - 1 + i] = Ux[i][k1][2];
						}
						else if (Bc.face == 3) {	//boundary i+
							B.x[iend2 + i + LAP][jbegin2 + k - 1 + LAP] = Ux[i][k1][1];
							B.y[iend2 + i + LAP][jbegin2 + k - 1 + LAP] = Ux[i][k1][2];
						}
						else {			// boundary j+
							B.x[ibegin2 + k - 1 + LAP][jend2 + i + LAP] = Ux[i][k1][1];
							B.y[ibegin2 + k - 1 + LAP][jend2 + i + LAP] = Ux[i][k1][2];
						}
					}
				}
				deleteMatrix(Ux, LAP, n1);
			}
			else {	//not inner boundary
					//���ڱ߽�㣬Ghost Cell ������������Ʒ�����ã��д��Ľ���
				for (int i = 1; i <= LAP; ++i) {
					if (Bc.face == 1) {	//blundary i-
						for (int j = jbegin2; j <= jend2; ++j) {
							B.x[ibegin2 - i + LAP][j + LAP] = 2.0*B.x[ibegin2 + LAP][j + LAP] - B.x[ibegin2 + i + LAP][j + LAP];
							B.y[ibegin2 - i + LAP][j + LAP] = 2.0*B.y[ibegin2 + LAP][j + LAP] - B.y[ibegin2 + i + LAP][j + LAP];
						}
					}
					else if (Bc.face == 2) {	//boundary j-
						for (int k = ibegin2; k <= iend2; ++k) {
							B.x[k + LAP][jbegin2 - i + LAP] = 2.0*B.x[k + LAP][jbegin2+LAP] - B.x[k + LAP][jbegin2 + i + LAP];
							B.y[k + LAP][jbegin2 - i + LAP] = 2.0*B.y[k + LAP][jbegin2+LAP] - B.y[k + LAP][jbegin2 + i + LAP];
						}
					}
					else if (Bc.face == 3) {	//boundary i+
						for (int j = jbegin2; j <= jend2; ++j) {
							B.x[iend2 + i + LAP][j + LAP] = 2.0*B.x[iend2 + LAP][j + LAP] - B.x[iend2 - i + LAP][j + LAP];
							B.y[iend2 + i + LAP][j + LAP] = 2.0*B.y[iend2 + LAP][j + LAP] - B.y[iend2 - i + LAP][j + LAP];
						}
					}
					else {			// boundary j+
						for (int k = ibegin2; k <= iend2; ++k) {
							B.x[k + LAP][jend2 + i + LAP] = 2.0*B.x[k + LAP][jend2 + LAP] - B.x[k + LAP][jend2 - i + LAP];
							B.y[k + LAP][jend2 + i + LAP] = 2.0*B.y[k + LAP][jend2 + LAP] - B.y[k + LAP][jend2 - i + LAP];
						}
					}
				}
			}
		}

		//conner point 
		int nx = B.nx;	int ny = B.ny;

		//����Ľǵ㣬һ��LAP*LAP��
		for (int i = 1; i <= LAP; ++i) {
			for (int j = 1; j <= LAP; ++j) {
				int i1 = 1 - i; int j1 = 1 - j;
				int i2 = nx + i; int j2 = ny + j;
				i1 = i1 + LAP;	j1 = j1 + LAP;
				i2 = i2 + LAP;	j2 = j2 + LAP;
				B.x[i1][j1] = B.x[i1][1 + LAP] + B.x[1 + LAP][j1] - B.x[1 + LAP][1 + LAP]; B.y[i1][j1] = B.y[i1][1 + LAP] + B.y[1 + LAP][j1 + LAP] - B.y[1 + LAP][1 + LAP];
				B.x[i2][j2] = B.x[i2][ny + LAP] + B.x[nx + LAP][j2] - B.x[nx + LAP][ny + LAP]; B.y[i2][j2] = B.y[i2][ny + LAP] + B.y[nx][j2] - B.y[nx + LAP][ny + LAP];
				B.x[i1][j2] = B.x[i1][ny] + B.x[1 + LAP][j2] - B.x[1 + LAP][ny]; B.y[i1][j2] = B.y[i1][ny] + B.y[1 + LAP][j2] - B.y[1 + LAP][ny];
				B.x[i2][j1] = B.x[i2][1 + LAP] + B.x[nx][j1] - B.x[nx][1 + LAP]; B.y[i2][j1] = B.y[i2][1 + LAP] + B.y[nx][j1] - B.y[nx][1 + LAP];
			}
		}

		//coordinate at the cell center
		for (int i = 1; i <= nx + 2 * LAP - 1; ++i) {
			for (int j = 1; j <= ny + 2 * LAP - 1; ++j) {
				B.x1[i][j] = (B.x[i][j] + B.x[i + 1][j] + B.x[i][j + 1] + B.x[i + 1][j + 1])*0.25;
				B.y1[i][j] = (B.y[i][j] + B.y[i + 1][j] + B.y[i][j + 1] + B.y[i + 1][j + 1])*0.25;
			}
		}

	}

	//�������������������Ϣ��tecplot ��ʽ��
	std::string filename = "mesh - test";
	std::string fileNumber = std::to_string(nMesh);
	filename = filename + fileNumber + ".plt";
	std::ofstream fcout;
	fcout.open(filename);
	fcout << "variables=x,y" << std::endl;
	for (int m = 1; m <= Mesh[nMesh].Num_Block; ++m) {
		Block_TYPE &B = Mesh[nMesh].Block[m];
		fcout << " zone  i=  " << B.nx+2*LAP  << "  j=  " << B.ny + 2*LAP << std::endl;
		for (int j = 1; j <= B.ny + 2*LAP; ++j) {
			for (int i = 1; i<=B.nx + 2*LAP; ++i) {
				fcout << B.x[i][j] << "  " << B.y[i][j] << std::endl;
				if (abs(B.x[i][j]) > 100) {
					printf("nx= %d, ny= %d, i=%d, j=%d, %f \n", B.nx, B.ny, i, j, B.x[i][j]);
					PAUSE;
				}
			}
		}
	}

	fcout.close();

}

//����߽����������ڱ߽磩 ������һ������
void Boundary_condition_onemesh(int nMesh) 
{
	for (int mBlock = 1; mBlock <= Mesh[nMesh].Num_Block; ++mBlock) {
		Block_TYPE & B = Mesh[nMesh].Block[mBlock];
		for (int ksub = 1; ksub <= B.subface; ++ksub) {
			BC_MSG_TYPE & Bc = B.bc_msg[ksub];
			if (Bc.neighb < 0) {	//���ڱ߽�
				if (Bc.neighb == BC_Wall) {//�̱ڱ߽�
					boundary_wall(nMesh, mBlock, ksub);
				}
				else if (Bc.neighb == BC_Farfield || Bc.neighb == BC_Inlet || Bc.neighb == BC_Outlet) {
					//Զ���߽� or ��� or ����
					boundary_Farfield_inlet_outlet(nMesh, mBlock, ksub, Bc.neighb);
				}
				else if (Bc.neighb == BC_Symmetry_or_slidewall) {	//�Գƣ����ƣ��߽�
					Symmetry_or_slidewall(nMesh, mBlock, ksub);
				}
				else if (Bc.neighb == BC_Move_Wall) {
					boundary_move_wall(nMesh, mBlock, ksub);
				}
			}

		}
	}
}

//-------------------------------------------------------------------------- -
//inner boundary condition  �ڱ߽�
//�����������ӹ�ϵ�����»�����������������Ϣ(����һ������)
//������ΪLAP������(Ŀǰ�汾�趨LAP = 2)

void update_buffer_onemesh(int nMesh)
{
	Mesh_TYPE & MP = Mesh[nMesh];
	int Nvar1 = MP.Nvar;
	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
		Block_TYPE & B = MP.Block[mBlock];
		for (int ksub = 1; ksub <= B.subface; ++ksub) {
			BC_MSG_TYPE & Bc = B.bc_msg[ksub];

			if (Bc.neighb >= 0) {	//inner boundary
				int ibegin2 = Bc.ist; int iend2 = Bc.iend; 
				int jbegin2 = Bc.jst; int jend2 = Bc.jend;       //write
				int n1 = iend2 - ibegin2 + jend2 - jbegin2;     //+ 1 (number of cells)

				//�����ӵ����Ϣ������ʱ����Ua
				double *** Ua;			//�洢������Ϣ����ʱ����
				allocMatrix(Ua,LAP, n1, Nvar1);

				int m_neighbour = Bc.neighb;  int msub = Bc.subface;
				Block_TYPE & B1 = Mesh[nMesh].Block[m_neighbour];
				BC_MSG_TYPE & Bc1 = B1.bc_msg[msub];

				int ibegin1 = Bc1.ist; int  iend1 = Bc1.iend; 
				int jbegin1 = Bc1.jst; int jend1 = Bc1.jend;      //read
				
				if (Bc1.face == 1) {   // boundary  i-
					for (int i = 1; i <= LAP; ++i){
						for (int j = jbegin1; j <= jend1 - 1; ++j) {
							for (int k = 1; k <= Nvar1; ++k) {
								//!Ua(:, j, i) = B1%U(:, ibegin1 + i - 1, j)
								Ua[i][j - jbegin1 + 1][k] = B.U[ibegin1 + i - 1+LAP][j+LAP][k];
							}
						}
					}
				}
				else if (Bc1.face == 2) {		// boundary  j-
					for (int i = ibegin1; i <= iend1-1; ++i) {
						for (int j = 1; j <= LAP; ++j) {
							for (int k = 1; k <= Nvar1; ++k) {
								//Ua(:, i, j) = B1%U(:, i, jbegin1 + j - 1)
								Ua[j][i - ibegin1 + 1][k] = B1.U[i+LAP][jbegin1 + j - 1+LAP][k];
							}
						}
					}
				}
				else if (Bc1.face == 3) {		// boundary  i+
					for (int i = 1; i <= LAP; ++i) {
						for (int j = jbegin1; j <= jend1 - 1; ++j) {
							for (int k = 1; k <= Nvar1; ++k) {
								//Ua(:, j, i) = B1%U(:, iend1 - LAP - 1 + i, j)
								Ua[i][j - jbegin1 + 1][k] = B1.U[iend1 - 1 + i][j+LAP][k];
							}
						}
					}
				}
				else {			/*boundary  j+*/
					for (int i = ibegin1; i <= iend1 - 1; ++i) {
						for (int j = 1; j <= LAP; ++j) {
							for (int k = 1; k <= Nvar1; ++k) {
								//Ua(:, i, j) = B1%U(:, i, jend1 - LAP - 1 + j)
								Ua[j][i - ibegin1 + 1][k] = B1.U[i+LAP][jend1 - 1 + j][k];
							}
						}
					}
				}

				//����ʱ����Ua�е���Ϣд�뻺����
				int orient = Bc.orient;          //orient == 2 ˳��;   ����ֵΪ ����
				int k1 = 1;
				for (int k = 1; k <= n1; ++k) {
					if (orient == 2) {
						k1 = k;
					}
					else {
						k1 = n1 + 1 - k;
					}

					if (Bc.face == 1) {		 //boundary  i-
						for (int i = 1; i <= LAP; ++i) {
							for (int invar = 1; invar <= Nvar1; ++invar) {
								B.U[ibegin2 - 1 + i][jbegin2 + k - 1+LAP][invar] = Ua[i][k1][invar];
							}
						}
					}
					else if (Bc.face == 2) {		//boundary  j-
						for (int j = 1; j <= LAP; ++j) {
							for (int invar = 1; invar <= Nvar1; ++invar) {
								B.U[ibegin2 + k - 1+LAP][jbegin2 - 1 + j][invar] = Ua[j][k1][invar];
							}
						}
					}
					else if (Bc.face == 3) {		//boundary  i+
						for (int i = 1; i <= LAP; ++i) {
							for (int invar = 1; invar <= Nvar1; ++invar) {
								B.U[iend2 + i - 1+LAP][jbegin2 + k - 1+LAP][invar] = Ua[i][k1][invar];
							}
						}
					}
					else {		//boundary  j+
						for (int j = 1; j <= LAP; ++j) {
							for (int invar = 1; invar <= Nvar1; ++invar) {
								B.U[ibegin2 + k - 1+LAP][jend2 + j - 1+LAP][invar] = Ua[j][k1][invar];
							}
						}
					}
				}
				deleteMatrix(Ua, LAP, n1);
			}
		}

		//���������Ľǵ㣬�ò�ֵ�ķ�����ֵ
		int nx1 = B.nx; int ny1 = B.ny;
		U_average_conner(Nvar1, B.U[1+LAP][LAP], B.U[1+LAP][1+LAP], B.U[LAP][1+LAP], B.U[LAP][LAP], Cv);
		U_average_conner(Nvar1, B.U[1+LAP][ny1+LAP], B.U[1+LAP][ny1 - 1+LAP], B.U[0+LAP][ny1 - 1+LAP], B.U[0+LAP][ny1+LAP], Cv);
		U_average_conner(Nvar1, B.U[nx1 - 1+LAP][0+LAP], B.U[nx1 - 1+LAP][1+LAP], B.U[nx1+LAP][1+LAP], B.U[nx1+LAP][0+LAP], Cv);
		U_average_conner(Nvar1, B.U[nx1 - 1+LAP][ny1+LAP], B.U[nx1 - 1+LAP][ny1 - 1+LAP], B.U[nx1+LAP][ny1 - 1+LAP], B.U[nx1+LAP][ny1+LAP], Cv);
	}
}

//!------------------------------------------------------------------ -
//!�������߽�����
//!ʹ��LAP��������
void boundary_wall(int nMesh, int mBlock, int ksub)
{
	const double beta1_SST = 0.075e0;     

	Mesh_TYPE & MP = Mesh[nMesh];
	Block_TYPE & B= MP.Block[mBlock];
	BC_MSG_TYPE & Bc = B.bc_msg[ksub];

	double Tsb = 110.4e0 / T_inf;    //�ο��¶� ��Surthland��ʽ��ʹ��)

	 //�������� U �� x ��y ����������������ƫ�ƣ��������������յ�Ҳ������Ӧ���޸ġ�
	int ibegin = Bc.ist+LAP;  int iend = Bc.iend + LAP;
	int jbegin = Bc.jst + LAP; int jend = Bc.jend + LAP;

	for (int k = 1; k <= LAP; ++k) {
		for (int i = ibegin; i <= iend; ++i) {
			for (int j = jbegin; j <= jend; ++j) {
				//(i1, j1)  �ڵ㣻(i2, j2) Ϊ��Ӧ��buffer���ĵ�
				int i1 = 0; int j1 = 0;
				int i2 = 0; int j2 = 0;
				if (Bc.face == 1) {
					i1 = i + k - 1; j1 = j; i2 = i - k; j2 = j;
				}
				else if (Bc.face == 2) {
					i1 = i; j1 = j + k - 1; i2 = i; j2 = j - k;
				}
				else if (Bc.face == 3) {
					i1 = i - k; j1 = j;  i2 = i + k - 1; j2 = j;
				}
				else {
					i1 = i; j1 = j - k; i2 = i; j2 = j + k - 1;
				}
				//i1 += LAP; j1 += LAP;
				//i2 += LAP; j2 += LAP;
				double d1=0;
				if (Twall <= 0) {  //���ȱ�
					B.U[i2][j2][1] = B.U[i1][j1][1];       //d(0) = d(1)   �Գ�
					B.U[i2][j2][2] = -B.U[i1][j1][2];      //u(0) = -u(1)->d*u ���Գ�
					B.U[i2][j2][3] = -B.U[i1][j1][3];      //v(0) = -v(1)->d*v ���Գ�
					B.U[i2][j2][4] = B.U[i1][j1][4];       //E(0) = E(1)   �Գ�
				}
				else {   //���±�
					d1 = B.U[i1][j1][1];   //�ڵ㴦���ܶȡ�ѹ�����¶ȡ��ٶ�
					double u1 = B.U[i1][j1][2] / d1;
					double v1 = B.U[i1][j1][3]/ d1;
					double p1 = (B.U[i1][j1][4] - 0.50*d1*(u1*u1 + v1*v1))*(gamma - 1.0);
					double T1 = gamma*Ma*Ma*p1 / d1;

					double p2 = p1;               //�߽����裬���洦����ѹ���ݶ�Ϊ0
					double T2 = 2.e0*Twall - T1;    //���±�  0.5*(T1 + T2) = Twall
					double u2 = -u1;              //�޻��Ʊ�
					double v2 = -v1;
					double d2 = gamma*Ma*Ma*p2 / T2;

					B.U[i2][j2][1] = d2;
					B.U[i2][j2][2] = d2*u2;
					B.U[i2][j2][3] = d2*v2;
					B.U[i2][j2][4] = p2 / (gamma - 1.0) + 0.50*d2*(u2*u2 + v2*v2);
				}

				if (MP.Nvar == 5) {
					B.U[i2][j2][5] = 0.e0;
				}
				if (MP.Nvar == 6) {
					double d2 = B.U[i2][j2][1];
					double T2 = (B.U[i2][j2][4] - 0.5e0*(B.U[i2][j2][2] * B.U[i2][j2][2] + B.U[i2][j2][3] * B.U[i2][j2][3]) / d2) / (Cv*d2);
					double Amu2 = (1.e0 + Tsb)*sqrt(T2*T2*T2) / (Tsb + T2);   //����ճ��ϵ����sutherland equation

					//B.U(5, i2, j2) = 0.d0                               //�Ķ��� ���̱���Ϊ0��
					//B.U(6, i2, j2) = 10.d0*6.d0*Amu2 / (d2*beta1_SST*B.dw(i1, j1)**2 * Re*Re)   //���ܱȺ�ɢ��, Bug removed 2012 - 5 - 10

					double wt = 60.e0*Amu2 / (d2*beta1_SST*B.dw[i1 - LAP][j1 - LAP] * B.dw[i1 - LAP][j1 - LAP] * Re*Re);
					B.U[i2][j2][5] = -B.U[i1][j1][5];           //���澵��㣬 ʹ�ñ�����k = 0
					B.U[i2][j2][6] = (d1 + d2)*wt - B.U[i1][j1][6];
				}
				//printf("  %d,  %d,\n  %f,  %f, %f,  %f \n", i2, j2, B.U[i2][j2][1], B.U[i2][j2][2], B.U[i2][j2][3], B.U[i2][j2][4]);
				//printf("  %d,  %d,\n  %f,  %f, %f,  %f \n", i1, j1, B.U[i1][j1][1], B.U[i1][j1][2], B.U[i1][j1][3], B.U[i1][j1][4]);
				////printf("%f \n", Ma_n);
				//PAUSE;
			}
		}
	}

}


//!------------------------------------------------------------------ -
//!�������߽�����,�������ٶ��˶�
//!ʹ��LAP��������
void boundary_move_wall(int nMesh, int mBlock, int ksub)
{
	const double beta1_SST = 0.075e0;
	const double U_wall = 1.0;
	Mesh_TYPE & MP = Mesh[nMesh];
	Block_TYPE & B = MP.Block[mBlock];
	BC_MSG_TYPE & Bc = B.bc_msg[ksub];

	double Tsb = 110.4e0 / T_inf;    //�ο��¶� ��Surthland��ʽ��ʹ��)

									 //�������� U �� x ��y ����������������ƫ�ƣ��������������յ�Ҳ������Ӧ���޸ġ�
	int ibegin = Bc.ist + LAP;  int iend = Bc.iend + LAP;
	int jbegin = Bc.jst + LAP; int jend = Bc.jend + LAP;

	for (int k = 1; k <= LAP; ++k) {
		for (int i = ibegin; i <= iend; ++i) {
			for (int j = jbegin; j <= jend; ++j) {
				//(i1, j1)  �ڵ㣻(i2, j2) Ϊ��Ӧ��buffer���ĵ�
				int i1 = 0; int j1 = 0;
				int i2 = 0; int j2 = 0;
				if (Bc.face == 1) {
					i1 = i + k - 1; j1 = j; i2 = i - k; j2 = j;
				}
				else if (Bc.face == 2) {
					i1 = i; j1 = j + k - 1; i2 = i; j2 = j - k;
				}
				else if (Bc.face == 3) {
					i1 = i - k; j1 = j;  i2 = i + k - 1; j2 = j;
				}
				else {
					i1 = i; j1 = j - k; i2 = i; j2 = j + k - 1;
				}
				//i1 += LAP; j1 += LAP;
				//i2 += LAP; j2 += LAP;
				double d1 = 0;
				if (Twall <= 0) {  //���ȱ�
					B.U[i2][j2][1] = B.U[i1][j1][1];       //d(0) = d(1)   �Գ�
					B.U[i2][j2][2] = 2 * U_wall*B.U[i1][j1][1] - B.U[i1][j1][2];      //u(0) = -u(1)->d*u ���Գ�
					B.U[i2][j2][3] = -B.U[i1][j1][3];      //v(0) = -v(1)->d*v ���Գ�
					double u2 = B.U[i2][j2][2] / B.U[i2][j2][1];	double u1 = B.U[i1][j1][2] / B.U[i1][j1][1];
					B.U[i2][j2][4] = B.U[i1][j1][4] + 0.5*B.U[i2][j2][1] * (u2*u2 - u1*u1);       //E(0) = E(1)   �Գ�
				}
				else {   //���±�
					d1 = B.U[i1][j1][1];   //�ڵ㴦���ܶȡ�ѹ�����¶ȡ��ٶ�
					double u1 = B.U[i1][j1][2] / d1;
					double v1 = B.U[i1][j1][3] / d1;
					double p1 = (B.U[i1][j1][4] - 0.50*d1*(u1*u1 + v1*v1))*(gamma - 1.0);
					double T1 = gamma*Ma*Ma*p1 / d1;

					double p2 = p1;               //�߽����裬���洦����ѹ���ݶ�Ϊ0
					double T2 = 2.e0*Twall - T1;    //���±�  0.5*(T1 + T2) = Twall
					double u2 = 2 * U_wall - u1;              //�޻������ٶȱ���
					double v2 = -v1;
					double d2 = gamma*Ma*Ma*p2 / T2;

					B.U[i2][j2][1] = d2;
					B.U[i2][j2][2] = d2*u2;
					B.U[i2][j2][3] = d2*v2;
					B.U[i2][j2][4] = p2 / (gamma - 1.0) + 0.50*d2*(u2*u2 + v2*v2);
				}

				if (MP.Nvar == 5) {
					B.U[i2][j2][5] = 0.e0;
				}
				if (MP.Nvar == 6) {
					double d2 = B.U[i2][j2][1];
					double T2 = (B.U[i2][j2][4] - 0.5e0*(B.U[i2][j2][2] * B.U[i2][j2][2] + B.U[i2][j2][3] * B.U[i2][j2][3]) / d2) / (Cv*d2);
					double Amu2 = (1.e0 + Tsb)*sqrt(T2*T2*T2) / (Tsb + T2);   //����ճ��ϵ����sutherland equation

																			  //B.U(5, i2, j2) = 0.d0                               //�Ķ��� ���̱���Ϊ0��
																			  //B.U(6, i2, j2) = 10.d0*6.d0*Amu2 / (d2*beta1_SST*B.dw(i1, j1)**2 * Re*Re)   //���ܱȺ�ɢ��, Bug removed 2012 - 5 - 10

					double wt = 60.e0*Amu2 / (d2*beta1_SST*B.dw[i1 - LAP][j1 - LAP] * B.dw[i1 - LAP][j1 - LAP] * Re*Re);
					B.U[i2][j2][5] = -B.U[i1][j1][5];           //���澵��㣬 ʹ�ñ�����k = 0
					B.U[i2][j2][6] = (d1 + d2)*wt - B.U[i1][j1][6];
				}
			}
		}
	}
}

//!------------------------------------------------------------------------ -
//!�ɴ���Զ���߽����� �Լ���� / ���ڱ߽�����
void boundary_Farfield_inlet_outlet(int nMesh, int mBlock, int ksub, int Bctype)
{
	//�����Ŀǰ����������������������Զ����
	double d_inf = 1.e0; double u_inf = 1.e0*cos(AoA); 
	double v_inf = 1.e0*sin(AoA); double p_inf = 1.e0 / (gamma*Ma*Ma);

	Mesh_TYPE &	MP = Mesh[nMesh];
	Block_TYPE & B = MP.Block[mBlock];
	BC_MSG_TYPE & Bc = B.bc_msg[ksub];
	int Nvar = MP.Nvar;

	//�������� U �� x ��y ����������������ƫ�ƣ��������������յ�Ҳ������Ӧ���޸ġ�
	int ibegin = Bc.ist+LAP; int iend = Bc.iend+LAP; 
	int jbegin = Bc.jst+LAP; int jend = Bc.jend+LAP;

	for (int i = ibegin; i <= iend; ++i) {
		for (int j = jbegin; j <= jend; ++j) {
			//(i1, j1) �ǿ����߽���ڵ㣬(i2, j2) �Ǳ߽����Ghost Cell��
			int i1 = 0; int j1 = 0;
			int i2 = 0; int j2 = 0;
			if (Bc.face == 1) {       //i - �棬
				i1 = i; j1 = j;
			}
			else if (Bc.face== 2) {  //j - ��
				i1 = i; j1 = j;
			}
			else if (Bc.face== 3) {  //i + ��(i = ibegin = iend = nx), i1 = i - 1 ���ڵ�, i2 = i = nx��Ghost Cell
				i1 = i - 1; j1 = j;
			}
			else {
				i1 = i; j1 = j - 1;
			}
			//i1 += LAP;	j1 += LAP;

			double d1 = B.U[i1][j1][1];   double u1 = B.U[i1][j1][2] / d1;    double  v1 = B.U[i1][j1][3] / d1;
			double p1 = (B.U[i1][j1][4] - 0.5e0*d1*(u1*u1 + v1*v1))*(gamma - 1.e0);              //�ڵ㴦��ֵ
			double c1 = sqrt(gamma*p1 / d1);
				
			//����߽� �� ������       
			double n1;	double n2;
			if (Bc.face == 1) {
				double dx = B.x[i][j + 1] - B.x[i][j];   double  dy = B.y[i][j + 1] - B.y[i][j];
				double s = sqrt(dx*dx + dy*dy);
				n1 = -dy / s; n2 = dx / s;    // �ⷨ��  
			}
			else if (Bc.face == 3) {
				double dx = B.x[i][j + 1] - B.x[i][j];    double dy = B.y[i][j + 1] - B.y[i][j];
				double s = sqrt(dx*dx + dy*dy);
				n1 = dy / s; n2 = -dx / s;    // �ⷨ��  
			}
			else if (Bc.face == 2) {
				double dx = B.x[i + 1][j] - B.x[i][j];  double dy = B.y[i + 1][j] - B.y[i][j];
				double s = sqrt(dx*dx + dy*dy);
				n1 = dy / s; n2 = -dx / s;      // �ⷨ�� 
			}
			else {
				double dx = B.x[i + 1][j] - B.x[i][j];   double dy = B.y[i + 1][j] - B.y[i][j];
				double s = sqrt(dx*dx + dy*dy);
				n1 = -dy / s; n2 = dx / s;        // �ⷨ�� 
			}
            
//			if(KS_Farfield == 0) {       // Զ������ʽ0 �������ճ������������
//			 Ma_n=(u_inf*n1+v_inf*n2)*Ma    // ������Զ���������� �������˳��ڻ����������㣬2012-5-23��
//            }else
			double Ma_n = (u1*n1 + v1*n2) / c1;    // ����Mach��
//            endif
			
			for (int k = 1; k <= LAP; ++k) {

				if (Bc.face== 1) {       //i - �棬
					i2 = i - k; j2 = j;
				}
				else if (Bc.face== 2) {  //j - ��
					i2 = i; j2 = j - k;
				}
				else if (Bc.face== 3) {  //i + ��(i = ibegin = iend = nx)][ i1 = i - 1 ���ڵ�][ i2 = i = nx��Ghost Cell
					i2 = i + k - 1; j2 = j;
				}
				else{
					i2 = i; j2 = j + k - 1;
				}
				//i2 += LAP;	j2 += LAP;
				//
				if (Bctype == BC_Outlet || (Bctype == BC_Farfield && Ma_n >= 0.e0)) {   //����
					if (Ma_n > 1.e0 || p_outlet < 0 || Bctype == BC_Farfield) {
						for (int ivar = 1; ivar <= Nvar; ++ivar) {
							B.U[i2][j2][ivar] = B.U[i1][j1][ivar];        //�����ٳ��ڣ�����(BC_Farfield ���������ƴ���)
						}
					}
					else {   //�����ٳ��ڣ�������ѹ
						double pb = p_outlet;
						double db = d1 + (p_outlet - p1) / (c1*c1);
						double ub = u1 + (p1 - p_outlet) / (d1*c1)*n1;
						double vb = v1 + (p1 - p_outlet) / (d1*c1)*n2;
						double p2 = 2.e0*pb - p1; double d2 = 2.e0*db - d1;
						double u2 = 2.e0*ub - u1; double v2 = 2.e0*vb - v1;
						B.U[i2][j2][1] = d2;
						B.U[i2][j2][2] = d2*u2;
						B.U[i2][j2][3] = d2*v2;
						B.U[i2][j2][4] = p2 / (gamma - 1.E0) + 0.5e0*d2*(u2*u2 + v2*v2);
						if (MP.Nvar >= 5) {             //����ģ���й���
							for (int ivar = 5; ivar <= MP.Nvar; ++ivar) {
								B.U[i2][j2][ivar] = B.U[i1][j1][ivar];
							}
						}

					}
				}
				else {      //���
					if (p_outlet<0 || Ma_n <= -1.e0) {   //���������(�趨P_outlet<0 Ĭ�ϰ����������������ںͳ��ڱ߽�)
						B.U[i2][j2][1] = d_inf;
						B.U[i2][j2][2] = d_inf*u_inf;
						B.U[i2][j2][3] = d_inf*v_inf;
						B.U[i2][j2][4] = p_inf / (gamma - 1.e0) + 0.5e0*d_inf*(u_inf*u_inf + v_inf*v_inf);

					}
					else {
						double pb = 0.5e0*(p1 + p_inf - d1*c1*((u_inf - u1)*n1 + (v_inf - v1)*n2));
						double db = d_inf + (pb - p_inf) / (c1*c1);
						double ub = u_inf - (p_inf - pb) / (d1*c1)*n1;
						double vb = v_inf - (p_inf - pb) / (d1*c1)*n2;
						double p2 = 2.e0*pb - p1;	double d2 = 2.e0*db - d1;
						double u2 = 2.e0*ub - u1;	double v2 = 2.e0*vb - v1;
						B.U[i2][j2][1] = d2;
						B.U[i2][j2][2] = d2*u2;
						B.U[i2][j2][3] = d2*v2;
						B.U[i2][j2][4] = p2 / (gamma - 1.e0) + 0.5e0*d2*(u2*u2 + v2*v2);

					}

					if (MP.Nvar == 5) {
						B.U[i2][j2][5] = vt_inf / Re;          //����vtֵ��ͨ���趨����ճ��ϵ����3 - 5����
					}
					else if (MP.Nvar == 6) {
						B.U[i2][j2][5] = d_inf*Kt_inf;      //����ֵ
						B.U[i2][j2][6] = d_inf*Wt_inf;
					}
				}

			}
		}
	}
}


//!------------------------------------------------------------------ -
//!�ԳƱ߽�����(���ƹ̱�)
//!ʹ��LAP��������
void Symmetry_or_slidewall(int nMesh, int mBlock, int ksub)
{
	Mesh_TYPE &	MP = Mesh[nMesh];
	Block_TYPE & B = MP.Block[mBlock];
	BC_MSG_TYPE & Bc = B.bc_msg[ksub];
	int Nvar = MP.Nvar;

	//�������� U �� x ��y ����������������ƫ�ƣ��������������յ�Ҳ������Ӧ���޸ġ�
	int ibegin = Bc.ist + LAP; int iend = Bc.iend + LAP;
	int jbegin = Bc.jst + LAP; int jend = Bc.jend + LAP;

	for (int i = ibegin; i <= iend; ++i) {
		for (int j = jbegin; j <= jend; ++j) {
			//����߽編����
			double n1 = 0; double n2 = 0; 
			if (Bc.face== 1 || Bc.face== 3) {    //i + or i - boundary
				double dx = B.x[i][j + 1] - B.x[i][j];    double  dy = B.y[i][j + 1] - B.y[i][j];
				double si = sqrt(dx*dx + dy*dy);
				n1 = dy / si; n2 = -dx / si;   //normal vector at(i][ j) or (I - 1 / 2][ J)
			}
			else{   //j + or j -
				double dx = B.x[i + 1][j] - B.x[i][j];    double  dy = B.y[i + 1][j] - B.y[i][j];
				double si = sqrt(dx*dx + dy*dy);
				n1 = -dy / si; n2 = dx / si;   //normal vector at i][ j + 1 / 2
			}

			for (int k = 1; k <= LAP; ++k) {
				int i1 = 0;	int j1 = 0;	int i2 = 0; int j2 = 0;
				if (Bc.face == 1) {
					i1 = i + k - 1; j1 = j; i2 = i - k; j2 = j;
				}
				else if (Bc.face == 2) {
					i1 = i; j1 = j + k - 1; i2 = i; j2 = j - k;
				}
				else if (Bc.face == 3) {
					i1 = i - k; j1 = j;  i2 = i + k - 1; j2 = j;
				}
				else {
					i1 = i; j1 = j - k; i2 = i; j2 = j + k - 1;
				}
				double Vn = B.U[i1][j1][2] * n1 + B.U[i1][j1][3] * n2;   //������

				B.U[i2][j2][1] = B.U[i1][j1][1];       //d(0) = d(1)   �Գ�
				B.U[i2][j2][2] = B.U[i1][j1][2] - 2.e0*Vn*n1;       //�������෴������������
				B.U[i2][j2][3] = B.U[i1][j1][3] - 2.e0*Vn*n2;       //�������෴������������
				B.U[i2][j2][4] = B.U[i1][j1][4];       //E(0) = E(1)   �Գ�

				if (MP.Nvar == 5) {
					B.U[i2][j2][5] = B.U[i1][j1][5];     //�������Գ�
				}
				else if (MP.Nvar == 6) {         //k ,  w
					B.U[i2][j2][5] = B.U[i1][j1][5];     //�������Գ�
					B.U[i2][j2][6] = B.U[i1][j1][6];
				}
			}

		}
	}
}

//------------------------------ -
//���㻺�����ǵ㴦��ֵ ������U1(0, 0)���ֵ���� ����巽������
void U_average_conner(int Nvar1, double U1[], double U2[], double U3[], double*  U4, double Cv)
{
	double d1 = U1[1]; double uu1 = U1[2] / d1; double v1 = U1[3] / d1; double T1 = (U1[4] - (uu1*U1[2] + v1*U1[3])*0.5e0) / (d1*Cv);	 //density, velocity, Temperature
	double d2 = U2[1]; double uu2 = U2[2] / d2; double v2 = U2[3] / d2; double T2 = (U2[4] - (uu2*U2[2] + v2*U2[3])*0.5e0) / (d2*Cv);
	double d3 = U3[1]; double uu3 = U3[2] / d3; double v3 = U3[3] / d3; double T3 = (U3[4] - (uu3*U3[2] + v3*U3[3])*0.5e0) / (d3*Cv);
	double d4 = d1 + d3 - d2; double uu4 = uu1 + uu3 - uu2; double v4 = v1 + v3 - v2; double T4 = T1 + T3 - T2;
	U4[1] = d4; U4[2] = d4*uu4; U4[3] = d4*v4; U4[4] = d4*(Cv*T4 + (uu4*uu4 + v4*v4)*0.5e0);
	
	if (Nvar1 == 5) {
		U4[5] = U1[5] + U3[5] - U2[5];
	}

	if (Nvar1 == 6) {
		U4[5] = U1[5] + U3[5] - U2[5];
		U4[6] = U1[6] + U3[6] - U2[6];
	}
}