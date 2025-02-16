#include"Serial.hpp"

#define READ_ERROR_COUNT 5000

using namespace std;

Serial::Serial()
{
	std::cout<< "serial start"<< std::endl;
}

Serial::~Serial()
{
	close_sci();
}

//シリアルポートの準備
int Serial::init(const char *port_name, int baud_rate, int data_bit, int stop_bit, int parity)
{
	fd = open(port_name, O_RDWR/* | O_NONBLOCK*/);
	if (fd < 0)
	{
		std::cerr << "Serial::open() : " << " not found." << std::endl;
		return -1;
	}
	
	struct termios tio;
	memset(&tio, 0, sizeof(tio));
	//シリアル通信の設定
	
	tio.c_cflag =  data_bit | stop_bit | parity | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR; 
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	
	tio.c_cc[VTIME] = 0;//キャラクタ間タイマ無効
	tio.c_cc[VMIN] = 0; //送信・受信時にブロックしない
	
	cfsetspeed(&tio, baud_rate);//ボーレートをセット

	tcflush(fd, TCIFLUSH);//バッファ内のデータをフラッシュ


	//新しいシリアルポート設定を適用
	tcsetattr(fd, TCSANOW, &tio);
	std::cout<<"Port Open!!"<<std::endl;
	rock = false;
	usleep(1000);
}

//シリアルポートのクローズ関数
int Serial::close_sci()
{
	tcflush(fd,TCIOFLUSH);	
	close(fd);
	return 0;
}

//受信関数(char型)
//受信に成功すれば「0」を返す。
//受信、または送信中ならば、ロックをかけておく。ロックがかかっていた場合は、-1を返す。
int Serial::receive_sci(unsigned char *data, int length, unsigned char *header, int header_size)
{
	//ロックの確認	
	if(rock == true)
	{	
		return -1;
	}
	
	rock = true;
	int ret = -1;//0より小さい値で初期化
	unsigned char test = 0;
	
	//アカデミーパックから返ってくる最初の文字は'@'なので、
	//受信したデータがアカデミーパックからのものかを確認する
	
	int count = 0;
	for(int i=0; i<header_size; i++)
	{
		while(test != header[i])
		{
			read(fd, &test, 1);
			count++;
			if(count > READ_ERROR_COUNT)
			{
				rock = false;
				//cout << "rock_false" << endl;
				return -2;
			}
		}	
		data[i] = test;
		//cout << "data[" << i << "] = " << data[i] << endl;
	}

	//cout << "true" << endl;
	//確認後受信
	do
	{
		//ret = read(fd, data+1, length);
		ret = read(fd, data + header_size, length);
	}while(ret < 0);
	
	rock = false;
	usleep(200);	
	return 0;
}
//送信関数(char型)
//送信に成功すれば「0」を返す。
//受信、または送信中ならば、ロックをかけておく。ロックがかかっていた場合は、-1を返す。
int Serial::send_sci(unsigned char*data, int length)
{
	int ret=-1;
	//ロックの確認
	if(rock == true)
	{	
		return -1;
	}
	
	//ロックをかけて、送信を行う
	rock = true;
	
	//すべてのデータを書き込むまでループ
	for (int wrlen = 0; wrlen < length ; wrlen += ret) 
	{
		//シリアルポートに書き込み 
		ret = write(fd, data + wrlen, length - wrlen);
		if (ret < 0) 
		{	ret = 0;	}
	}
	rock = false;
	
	usleep(200);	
	return 0;
}

