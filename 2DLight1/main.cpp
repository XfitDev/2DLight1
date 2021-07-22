#include <_system/_DXGI.h>

#include <system/Context.h>
#include <file/File.h>
#include <text/Font.h>
#include <sound/Sound.h>
#include <system/Input.h>
#include <math/Rect.h>
#include <physics/Collision.h>
#include <component/Button.h>
#include <physics/RectHitTest.h>
#include <decoder/PNGDecoder.h>
#include <object/LabelImage.h>
#include <object/ImageInstance.h>
#include <object/ImageMultiInstance.h>
#include <resource/FrameArray.h>
#include <time/Time.h>

#include <object/ScaleImage.h>

#include <resource/Index.h>

#include "XfitDataFiles.h"

ImageMultiInstance* imgIns;

using namespace std;

#ifdef _WIN32
static File file;
#elif __ANDROID__
static AssetFile file;
#endif

unsigned char* mainFileData,*imgData;



void LoadPNGImage(const unsigned int* _posSize, Frame* _outFrame, bool _noUseMipmap = false, FrameFormat _format = FrameFormat::RGBA) {
	PNGDecoder decoder;

	if (_format == FrameFormat::Height) {
		decoder.LoadHeader(&mainFileData[_posSize[0]], _posSize[1], ImageType::Gray);
	} else {
		decoder.LoadHeader(&mainFileData[_posSize[0]], _posSize[1]);
	}

	unsigned char* outData = new unsigned char[decoder.GetOutputSize()];
	decoder.Decode(outData);

	if (_noUseMipmap) {
		_outFrame->Build(outData, decoder.GetWidth(), decoder.GetHeight(), _format, 1);
	} else {
#ifdef __ANDROID__
		_outFrame->Build(outData, decoder.GetWidth(), decoder.GetHeight(), _format, 3);
#else
		_outFrame->Build(outData, decoder.GetWidth(), decoder.GetHeight(), _format, 3);
#endif
	}

	delete[]outData;
}

Frame* LoadPNGImageGetFrame(const unsigned int* _posSize, bool _noUseMipmap = false, FrameFormat _format = FrameFormat::RGBA) {
	Frame* outFrame = new Frame;
	LoadPNGImage(_posSize, outFrame, _noUseMipmap, _format);
	return outFrame;
}

void LoadCompImage(const unsigned int* _posSize, unsigned _width, unsigned _height, Frame* _outFrame, bool _noUseMipmap /*= false*/, FrameCompressFormat _format /*= FrameCompressFormat::BC3_RGBA*/) {
	if (_noUseMipmap) {
		_outFrame->BuildCompress(&mainFileData[_posSize[0]], _posSize[1], _width, _height, _format, false, 1);
	} else {
#ifdef __ANDROID__
		_outFrame->BuildCompress(&mainFileData[_posSize[0]], _posSize[1], _width, _height, _format, false, 3);
#else
		_outFrame->BuildCompress(&mainFileData[_posSize[0]], _posSize[1], _width, _height, _format, false, 3);
#endif
	}
}

Frame* LoadCompImageGetFrame(const unsigned int* _posSize, unsigned _width, unsigned _height, bool _noUseMipmap /*= false*/, FrameCompressFormat _format /*= FrameCompressFormat::BC3_RGBA*/) {
	Frame* outFrame = new Frame;
	LoadCompImage(_posSize, _width, _height, outFrame, _noUseMipmap, _format);
	return outFrame;
}

void LoadPNG(const unsigned* _path, unsigned char* _data) {
	PNGDecoder decoder;

	decoder.LoadHeader(&mainFileData[_path[0]], _path[1]);

	decoder.Decode(_data);
}

void LoadPNGImageArray(const unsigned int** _posSizes, unsigned _len, FrameArray* _outFrameArray, bool _noUseMipmap = false, FrameFormat _format = FrameFormat::RGBA) {
	unsigned char* outData = nullptr;

	ImageType imgType = ImageType::RGBA;
	if (_format == FrameFormat::Height) {
		imgType = ImageType::Gray;
	}
	unsigned width, height;

	for (unsigned i = 0; i < _len; i++) {//이미지 갯수만큼 불러와서 디코딩
		PNGDecoder decoder;

		decoder.LoadHeader(&mainFileData[_posSizes[i][0]], _posSizes[i][1], imgType);//PNG 헤더 불러오기
		

		if (i == 0) {
			outData = new unsigned char[decoder.GetOutputSize() * _len];//최초로 불러올때 이미지 갯수 총 크기만큼 메모리 할당
			width = decoder.GetWidth();//이미지 너비 높이 저장
			height = decoder.GetHeight();
		}
		decoder.Decode(outData + decoder.GetOutputSize()*i);//디코딩(전체 데이터에 디코딩한 이미지를 차례대로 배치)
	}

	

	if (_noUseMipmap) {//밉맵을 사용하지 않을경우
		_outFrameArray->Build(outData, _len, width, height, _format, 1);//이미지 데이터를 그래픽카드 메모리로 보냄.
	} else {
#ifdef __ANDROID__
		_outFrameArray->Build(outData, _len, width, height, _format, 3);
#else
		_outFrameArray->Build(outData, _len, width, height, _format, 3);
#endif
	}

	delete[]outData;
}
FrameArray* LoadPNGImageGetFrameArray(const unsigned int** _posSizes, unsigned _len, bool _noUseMipmap = false, FrameFormat _format = FrameFormat::RGBA) {
	FrameArray* outFrame = new FrameArray;
	LoadPNGImageArray(_posSizes, _len, outFrame, _noUseMipmap, _format);
	return outFrame;
}

ImageMultiInstance* tiles;
ScaleImage* character;

unsigned mapWidth, mapHeight;
unsigned tileSize;

void Main_Size();

#include <math/ALine.h>


Array<ALine> lines;
Array<ALine>* screenlines;

static void Init() {
	Time::Init();

	file.Open("Data.xfitData");//데이터 파일 불러옴

	unsigned size = file.GetSize();
	mainFileData = new unsigned char[size];
	file.ReadBytes(size, mainFileData);
	file.Close();//파일 읽어서 메모리에 저장하고 닫음
	 
	originalWindowWidth = 3840.f;//캔버스 사이즈 설정
	originalWindowHeight = 2160.f;

	InitWindowRatio();

	const unsigned* posSizes[2] = {FileData::Map::Brick_png, FileData::Map::WoodTile_png};
	FrameArray* tilesFrame = LoadPNGImageGetFrameArray(posSizes, 2);

	PNGDecoder decoder;

	decoder.LoadHeader(&mainFileData[FileData::Map::map_png[0]], FileData::Map::map_png[1], ImageType::Gray);
	unsigned short* outData = new unsigned short[decoder.GetOutputSize() / 2];
	decoder.Decode(outData);

	mapWidth = decoder.GetWidth();//맵 크기 가져옴
	mapHeight = decoder.GetHeight();
	tileSize = tilesFrame->GetWidth();

	tiles = new ImageMultiInstance(PointF(0, 0), PointF(1,1), 0, nullptr, System::defaultSampler, tilesFrame, System::defaultVertex2D, System::defaultUV, System::defaultIndex);


	tiles->nodes.Alloc(mapWidth * mapHeight);//맵 타일 수만큼 할당
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			MultiInstanceNode node;
			node.colorMat.Identity();
			node.imgIndex = outData[y * mapHeight + x] ? 1 : 0;
			node.mat = Matrix::GetMatrix2D((x - (float)mapWidth / 2.f)* (float)tileSize, -(y - (float)mapHeight / 2.f)* (float)tileSize, tileSize, tileSize, 0);
			tiles->nodes.InsertLast(node);
		}
	}
	tiles->BuildInstance();

	lines.Alloc(100);
	for (int y = 1; y < mapHeight; y++) {
		ALine l = { PointF(-1,-1),PointF(-1,-1) };
		for (int x = 1; x < mapWidth; x++) {
			if (lines.Size() >= lines.MaxSize()) {
				lines.ReAlloc(lines.MaxSize() + 100);
			}
			if ((outData[y * mapHeight + x] == 65535 && outData[(y - 1) * mapHeight + x] == 0) || (outData[y * mapHeight + x] == 0 && outData[(y - 1) * mapHeight + x] == 65535)) {
				if (l.start.x == -1) {
					l.start = PointF(x * (float)tileSize, -y * (float)tileSize);
				}
			} else {
				if (l.start.x != -1) {
					l.end = PointF(x * (float)tileSize, -y * (float)tileSize);
					lines.InsertLast(l);
					l.start = PointF(-1, -1);
					l.end = PointF(-1, -1);
				}
			}
			
		}
	}
	for (int x = 1; x < mapWidth; x++) {
		ALine l = { PointF(-1,-1),PointF(-1,-1) };
		for (int y = 1; y < mapHeight; y++) {
			if (lines.Size() >= lines.MaxSize()) {
				lines.ReAlloc(lines.MaxSize() + 100);
			}
			if ((outData[y * mapHeight + x] == 65535 && outData[y * mapHeight + x - 1] == 0) || (outData[y * mapHeight + x] == 0 && outData[y * mapHeight + x - 1] == 65535)) {
				if (l.start.x == -1) {
					l.start = PointF(x * (float)tileSize, -y * (float)tileSize);
				}
			} else {
				if (l.start.x != -1) {
					l.end = PointF(x * (float)tileSize, -y * (float)tileSize);
					lines.InsertLast(l);
					l.start = PointF(-1, -1);
					l.end = PointF(-1, -1);
				}
			}

		}
	}
	screenlines = &tiles->lines;
	screenlines->Alloc(lines.Size());


	character = new ScaleImage(PointF(0, 0), PointF(1, 1), 0, CenterPointPos::Center, false, System::defaultBlend, System::defaultSampler, LoadPNGImageGetFrame(FileData::Map::Character_png), System::defaultUV, System::defaultIndex);


	Main_Size();
	tiles->SetPos(PointF((float)tileSize*(mapWidth/2-1)*1.5, -(float)tileSize * (mapHeight / 2 - 1)*1.5));
}

float mx = 0, my = 0;
float tt = 0.f;

float speed = 1.5f;

static void Update() {
	float mmx = 0,mmy = 0;
	if (Input::IsKeyPressing(Input::Key::A)) {
		mx -= speed * System::GetDeltaTime();
		mmx = speed * System::GetDeltaTime();
	}
	if (Input::IsKeyPressing(Input::Key::D)) {
		mx += speed * System::GetDeltaTime();
		mmx = -speed * System::GetDeltaTime();
	}
	if (Input::IsKeyPressing(Input::Key::W)) {
		my += speed * System::GetDeltaTime();
		mmy = -speed * System::GetDeltaTime();
	}
	if (Input::IsKeyPressing(Input::Key::S)) {
		my -= speed * System::GetDeltaTime();
		mmy = speed * System::GetDeltaTime();
	}

	float halfSize = tileSize / 2;
	
	PointF cPos = PointF(tileSize + halfSize + (float)mx* tileSize, - ((float)tileSize + halfSize) + (float)my* tileSize - 10);

	screenlines->Clear();
	float left = -(float)System::GetWindowWidth() / 2.f / WindowRatio() * 1.5;
	float right = -left;
	float top = (float)System::GetWindowHeight() / 2.f / WindowRatio() * 1.5;
	float bottom = -top;
	RectF windowRect = RectF(left, right, top, bottom).Move(mx*tileSize,my * tileSize);

	for (int i = 0; i < lines.Size(); i++) {
		if (CollisionF::CollisionRectInLine(windowRect, lines[i].start, lines[i].end, false)) {
			screenlines->InsertLast(lines[i]);
		}
	}

	for (int i = 0; i < screenlines->Size(); i++) {
		PointF outPt = PointF(0,0);
		if (CollisionF::CollisionCircleInLine((*screenlines)[i].start, (*screenlines)[i].end, cPos,49,&outPt)) {
			mx += mmx;
			my += mmy;
			break;
		}
	}
	for (int i = 0; i < screenlines->Size(); i++) {
		(*screenlines)[i].start -= PointF(tileSize + halfSize + (float)mx * tileSize, -((float)tileSize + halfSize) + (float)my * tileSize);
		(*screenlines)[i].end -= PointF(tileSize + halfSize + (float)mx * tileSize, -((float)tileSize + halfSize) + (float)my * tileSize);
		(*screenlines)[i].start *= WindowRatio()*1.5;
		(*screenlines)[i].end *= WindowRatio() * 1.5;
	}


	PointF tilePos = PointF((float)tileSize * (mapWidth / 2 - 1 - mx) * 1.5, -(float)tileSize * (mapHeight / 2 - 1 + my) * 1.5);

	tiles->SetPos(tilePos);
	tiles->lightPos = PointF(0,0);
	tiles->lightDir = Input::GetMousePos();
	tiles->lightPower = 150;
	tiles->lightAngle = 90 * Math::DIVPI_180F;
	character->SetRotation(-tiles->lightDir.GetAngle(PointF(0,0)) + 90.f);


	tt += System::GetDeltaTime();


	if (!System::IsPause()) {
		System::Clear(false);

		tiles->Draw();
		character->Draw();

		System::Render();
	} else {

		System::Wait(16);
	}
}

static void Activate() {
}
static void Destroy() {
	Font::Release();
}
void Main_Size() {
	PointF ratio(1.5, 1.5);

	tiles->SetScale(ratio);
	character->SetScale(ratio);
}

static void Move() {
}


static bool Closing() {
	return true;
}

static void Create() {
	System::CreateInfo createInfo;

	createInfo.refleshRateTop = 0;
	createInfo.refleshRateBottom = 1;

#ifdef _WIN32
	createInfo.windowPos.x = System::WindowDefaultPos;
	createInfo.windowPos.y = System::WindowDefaultPos;

	createInfo.screenMode = System::ScreenMode::Window;
	createInfo.windowSize.width = 1280;
	createInfo.windowSize.height = 720;
	createInfo.windowShow = System::WindowShow::Default;

	createInfo.title = _T("3D Test");
	createInfo.cursorResource = nullptr;
	createInfo.minimized = true;
	createInfo.maximized = true;
	createInfo.resizeWindow = true;

	createInfo.iconResource = NULL;
#endif

	createInfo.msaaCount = 1;
	createInfo.msaaQuality = 0;
	createInfo.vSync = true;
	createInfo.maxFrame = 0;
	createInfo.screenIndex = 0;

	System::updateFuncs = Update;
	System::activateFunc = Activate;
	System::destroyFunc = Destroy;
	System::sizeFunc = Main_Size;
	System::moveFunc = Move;
	System::closingFunc = Closing;

	System::Init(&createInfo);

	Font::Init(30000, 30000);

	System::SetClearColor(0.f, 0.f, 0.f, 1.f);

	Init();
}

#ifdef _WIN32
int APIENTRY _tWinMain(HINSTANCE _hInstance, HINSTANCE, LPTSTR, int) {
	System::createFunc = Create;

	System::Create(_hInstance);

	return 0;
}
#elif __ANDROID__
void android_main(struct android_app* state) {
	System::createFunc = Create;

	Main(state);
}
#endif