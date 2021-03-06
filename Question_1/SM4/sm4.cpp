#include "sm4.h"

unsigned int iv[4] = {rand(), rand(), rand(), rand()};
unsigned int MK[4] = {rand(), rand(), rand(), rand()};
unsigned int plaintext[512];
unsigned int ciphertext[512];
unsigned int rk[32] = {};

unsigned int substitude(unsigned int in) {
    unsigned int out = 0;
    int i = 0;
    for(; i < 4; i++) {
        out = (out << 8) + (SBox[(unsigned char)(in >> (24 - 8 * i))]);
    }
    return out;
}

void keyExpansion(bool selection) { // true for encrypt, false for decrypt
  unsigned int K[4];
  for(int i = 0; i < 4; i++) {
    K[i] = MK[i] ^ FK[i];
  }
  for(int i = 0; i < 32; i++) {
    unsigned int t = substitude(K[1] ^ K[2] ^ K[3] ^ CK[i]);
    unsigned int l = t ^ ((t << 13) | (t >> 19)) ^ ((t << 23) | (t >> 9));
    rk[i] = K[0] ^ l;
    for(int j = 0; j < 3; j++) {
      K[j] = K[j + 1];
    }
    K[3] = rk[i];
  }
  if(!selection) {
    for(int i = 0; i < 16; i++) {
      unsigned int tmp = rk[i];
      rk[i] = rk[31 - i];
      rk[31 - i] = tmp;
    }
  }
}

void SM4(unsigned int* in, unsigned int* out) {
  unsigned int X[4] = {};
  for(int i = 0; i < 4; i++) {
    X[i] = in[i];
  }
  for(int i = 0; i < 32; i++) {
    unsigned int t = substitude(X[1] ^ X[2] ^ X[3] ^ rk[i]);
    unsigned int l = t ^ ((t << 2) | (t >> 30)) ^ ((t << 10) | (t >> 22)) ^ ((t << 18) | (t >> 14)) ^ ((t << 24) | (t >> 8));
    l = X[0] ^ l;
    for(int j = 0; j < 3; j++) {
      X[j] = X[j + 1];
    }
    X[3] = l;
  }
  for(int i = 0; i < 4; i++) {
    out[i] = X[3 - i];
  }
}

void SM4_core(unsigned int* X) {
  for(int i = 0; i < 32; i++) {
    unsigned int t = substitude(X[1] ^ X[2] ^ X[3] ^ rk[i]);
    unsigned int l = t ^ ((t << 2) | (t >> 30)) ^ ((t << 10) | (t >> 22)) ^ ((t << 18) | (t >> 14)) ^ ((t << 24) | (t >> 8));
    l = X[0] ^ l;
    for(int j = 0; j < 3; j++) {
      X[j] = X[j + 1];
    }
    X[3] = l;
  }
}

void encrypt() {
  int textLength = 512;
  int n = textLength / 4;
  unsigned int X[4];
  unsigned int cbc_saved[4] = {0, 0, 0, 0};
  keyExpansion(true);
  for(int l = 0; l < n; l ++) {
    for (int i = 0; i < 4; i ++) {
      X[i] = plaintext[l*4+i];
    }
    if (l == 0) {
      for (int i = 0; i < 4; i ++) {
        X[i] = X[i] ^ iv[i];
      }
    } else {
      for (int i = 0; i < 4; i ++) {
        X[i] = X[i] ^ cbc_saved[i];
      }
    }
    SM4_core(X);
    for(int i = 0; i < 4; i++) {
      ciphertext[l*4+i] = cbc_saved[i] = X[3 - i];
    }
  }
}

void decrypt() {
  int textLength = 512;
  int n = textLength / 4;
  unsigned int X[4];
  unsigned int cbc_saved[4] = {0, 0, 0, 0};
  keyExpansion(false);
  for (int i = 0; i < 4; i ++) {
    X[i] = ciphertext[i+4*(n-1)];
  }
  for(int l = n-1; l >= 0; l --) {
    SM4_core(X);
    for(int i = 0; i < 4; i++) {
      cbc_saved[i] = X[3 - i];
    }
    
    if (l == 0) {
      for (int i = 0; i < 4; i ++) {
        plaintext[l*4+i] = cbc_saved[i] = cbc_saved[i] ^ iv[i];
      }
    } else {
      for (int i = 0; i < 4; i ++) {
        X[i] = ciphertext[(l-1)*4+i];
        plaintext[l*4+i] = cbc_saved[i] = X[i] ^ cbc_saved[i];
      }
    }
  }
}

bool judge(unsigned int text1[], unsigned int text2[], int size) {
  for (int i = size - 1; i >= 0; i --) {
	if (text1[i] != text2[i]) {
	  cout << "error at: " << i << endl;
	  return false;
	}
  }
  return true;
}

int main() {
  string srcPath = "../../Data/plaintext.txt";
  string dstPath = "../../Data/ciphertext.txt";
  string text = "";
  ifstream textFile(srcPath);
  if (!textFile.is_open()) {
    cout << "can't open src file" << endl;
    return 0;
  }
  getline(textFile, text);
  unsigned char tmp1[2048];
  unsigned int tmp[512];
  strcpy((char*)tmp1, text.c_str());

  for (int i = 0; i < 512; i ++) {
    plaintext[i] = 
      (((unsigned int)(tmp1[4*i]-'0')) << 12)
      + (((unsigned int)(tmp1[4*i+1]-'0')) << 8)
      + (((unsigned int)(tmp1[4*i+2]-'0')) << 4)
      + (((unsigned int)(tmp1[4*i+3]-'0')));
    tmp[i] = plaintext[i];
  }

  clock_t encryptBegin = clock();	
  encrypt();
  clock_t encryptEnd = clock();

  clock_t decryptBegin = clock();
  decrypt();
  clock_t decryptEnd = clock();

  if(judge(tmp, plaintext, 512)) {
    cout << "SM4 Key: ";
		for (int i = 0; i < 4; i ++) {
			cout << MK[i];
		}
		cout << endl;

		cout << "SM4 IV: ";
		for (int i = 0; i < 4; i ++) {
			cout << iv[i];
		}
		cout << endl;

	  double duration = double(encryptEnd - encryptBegin) / CLOCKS_PER_SEC;
    cout << "SM4 ENCRYPT duration: " << duration << endl;
    cout << "SM4 ENCRYPT bandwidth: " << 1.0 / 64.0 / duration << "Mbps" << endl;
    
    duration = double(decryptEnd - decryptBegin) / CLOCKS_PER_SEC;
    cout << "SM4 DECRYPT duration: " << duration << endl;
    cout << "SM4 DECRYPT bandwidth: " << 1.0 / 64.0 / duration << "Mbps" << endl;
  } else {
    cout << "SM4 encryption and decryption error" << endl;
  }

  return 0;
}