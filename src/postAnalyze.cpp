#include "common.h"
#include "Global_Var.h"
#include "postAnalyze.h"
#include <fstream>
#include <iomanip>





//����̱��ϵ�ѹ��
void outputPressureOnWall(int nmesh)
{
	const double p00 = 1.e0 / (gamma*Ma*Ma);
	std::ofstream fcout;
	fcout.open("pressure.plt");
	fcout << std::setprecision(15);
	fcout << "VARIABLES=   \"x\" ,    \"pres\"   " << std::endl;

	Mesh_TYPE & MP = Mesh[1];
	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
		Block_TYPE & B = MP.Block[mBlock];
		for (int ksub = 1; ksub <= B.subface; ++ksub) {
			BC_MSG_TYPE & Bc = B.bc_msg[ksub];
			if (Bc.neighb == BC_Wall) {//�̱ڱ߽�
				int ist = Bc.ist + LAP;	int iend = Bc.iend + LAP;
				int jst = Bc.jst + LAP;	int jend = Bc.jend + LAP;
				for (int i = ist; i <= iend; ++i) {
					for (int j = jst; j <= jend; ++j) {
						double den = B.U[i][j][1];
						double uu = B.U[i][j][2] / den;
						double vv = B.U[i][j][3] / den;
						double T = (B.U[i][j][4] - 0.5e0*den* (uu * uu + vv * vv)) / (Cv*den);
						double Pre = p00*den * T;
						fcout << B.x1[i][j] << "  " << Pre << std::endl;
					}
				}
			}
		}
	}
	fcout.close();
}

//���ڷ�ǻ���������������  x=0.5�� �������ϵ��ٶ�
void outPutVelcoity()
{
	std::ofstream fcout;
	fcout.open("velocity.plt");
	fcout << std::setprecision(15);
	fcout << "VARIABLES=   \"y\" ,    \"Ux\"   " << std::endl;

	Mesh_TYPE & MP = Mesh[1];
	for (int mBlock = 1; mBlock <= MP.Num_Block; ++mBlock) {
		Block_TYPE & B = MP.Block[mBlock];
		int nx = B.nx;	int ny = B.ny;
		int myi = (int)(nx / 2);
		for (int j = 1 + LAP; j <= ny + LAP; ++j) {
			fcout << std::setw(9) << B.y1[myi][j] << "   " << std::setw(9) << B.U[myi][j][2] / B.U[myi][j][1] << std::endl;
		}
	}
	fcout.close();
}