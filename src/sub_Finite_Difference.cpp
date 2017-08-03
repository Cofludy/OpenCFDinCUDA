//�ò�ַ�����ͨ�� ���൱�ڲ�������
//������������£��������ܵ�������� ��ϡ����ͨ�������ø߾��ȷ�����

#include "Global_var.h"
#include "sub_Finite_Difference.h"
#include "common.h"
#include <fstream>
#include<iomanip>

void comput_Jacobin();


//���޲��ģ��ĳ�ʼ��
void Init_FiniteDifference()
{
	std::ifstream fcin;
	fcin.open("FDM.in");
	if (!fcin) {
		printf("Not find 'FDM.in', all blocks using Finite Volume Method\n");
		//PAUSE;
		//exit(1);
	}
	else {

	}

	comput_Jacobin();
}

//
void comput_Jacobin()
{
	std::ofstream fcout;
	fcout.open("");
	fcout << std::setprecision(12);
	fcout << "variables=x,y,kx,ky,ix,iy,Jac,s1" << std::endl;
	
	for (int m = 1; m <= Mesh[1].Num_Block; ++m) {
		Block_TYPE &B = Mesh[1].Block[m];
		if (B.FVM_FDM != Method_FDM) { continue; }
		
		// ����ط��ĳ���û�м���
		printf("can not computer the Jacobin cooeficient!\n");
		PAUSE;
	}




	fcout.close();
}
