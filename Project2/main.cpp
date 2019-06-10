#include <string>
#include <iostream>

#include <random>
#include <deque>
#include "wtypes.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include "Config.h"

#include "StarFallVis.h"
#include "TesseractVis.h"
#include "VortexVis.h"
#include "DoorwayVis.h"
#include "AngelVis.h"
#include "BetweenVis.h"


#include <fstream>

#include <Windows.h>
#include <Dwmapi.h>
#pragma comment (lib, "Dwmapi.lib")

Config* gameConfig;

float mean(float a, float b) { return a + (b-a)*0.5;}
float between(float a, float b) { return a*0.5f + b*0.5f; }

void four1(float* data, unsigned long nn)
{
	unsigned long n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;
	double tempr, tempi;

	// reverse-binary reindexing
	n = nn << 1;
	j = 1;
	for (i = 1; i < n; i += 2) {
		if (j > i) {
			std::swap(data[j - 1], data[i - 1]);
			std::swap(data[j], data[i]);
		}
		m = nn;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	};

	// here begins the Danielson-Lanczos section
	mmax = 2;
	while (n > mmax) {
		istep = mmax << 1;
		theta = -(2 * PI / mmax);
		wtemp = sin(0.5*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi = sin(theta);
		wr = 1.0;
		wi = 0.0;
		for (m = 1; m < mmax; m += 2) {
			for (i = m; i <= n; i += istep) {
				j = i + mmax;
				tempr = wr*data[j - 1] - wi*data[j];
				tempi = wr * data[j] + wi*data[j - 1];

				data[j - 1] = data[i - 1] - tempr;
				data[j] = data[i] - tempi;
				data[i - 1] += tempr;
				data[i] += tempi;
			}
			wtemp = wr;
			wr += wr*wpr - wi*wpi;
			wi += wi*wpr + wtemp*wpi;
		}
		mmax = istep;
	}
}

static int recordCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	int checkSize = framesPerBuffer;

	int numChannels = gameConfig->params.channelCount;

	if (gameConfig->frames.size() > checkSize)
		gameConfig->frames.erase(gameConfig->frames.begin(), gameConfig->frames.begin() + checkSize);

	SAMPLE *rptr = (SAMPLE*)inputBuffer;

	int s = 0;

	while (s < checkSize)
	{
		SAMPLE splLeft = (*rptr++);
		SAMPLE splRight = splLeft;
		if (numChannels == 2)
			splRight = (*rptr++);

		SAMPLE spl = (splLeft + splRight) / 2.f;

		//if ((float)spl < 0)
		   // spl *= -1;

	   // if (spl > gameConfig->frameHi && spl > gameConfig->cutoff)
	   //	 gameConfig->frameHi = spl;

		gameConfig->frames.push_back(fabs(spl));
		s++;
	}

	gameConfig->FFTdata = std::vector<SAMPLE>(gameConfig->frames.begin(), gameConfig->frames.begin() + (framesPerBuffer * 2));

	four1(gameConfig->FFTdata.data(), gameConfig->FFTdata.size() / 2);

	float factor = 20;

	{ //lock for frequency data
		std::lock_guard<std::mutex> guard(gameConfig->FreqDataMutex);

		gameConfig->FrequencyData.clear();
		for (int it = 0; it < gameConfig->FFTdata.size() / 2; it++)
		{

			auto re = gameConfig->FFTdata[it * 2];
			auto im = gameConfig->FFTdata[it * 2 + 1];
			auto magnitude = std::sqrt(re*re + im*im);
			//auto magnitude_dB = 20 * log10(magnitude);
			float point = it / factor + 0.3;
			magnitude = magnitude*atan(point);
			gameConfig->FrequencyData.push_back(magnitude);

			//store data for bass and treble
			if (it < 15 && magnitude > gameConfig->bassHi)
				gameConfig->bassHi = magnitude;

			if (it > framesPerBuffer / 4 && it < framesPerBuffer / 2 && magnitude > gameConfig->trebleHi)
				gameConfig->trebleHi = magnitude;

			if (magnitude > gameConfig->frameHi && magnitude > gameConfig->cutoff)
				gameConfig->frameHi = magnitude;
		}
	} //end lock
	return paContinue;
}

void getWindowSizes()
{
	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);

	gameConfig->fullScrW = desktop.right;
	gameConfig->fullScrH = desktop.bottom;

	gameConfig->minScrW = SCRW;
	gameConfig->minScrH = SCRH;

	gameConfig->scrW = gameConfig->minScrW;
	gameConfig->scrH = gameConfig->minScrH;

	gameConfig->ratio = gameConfig->scrW / gameConfig->scrH;
}

void initWindow(bool firstStart = false)
{
	if (gameConfig->isFullScreen)
	{
		gameConfig->minScrW = gameConfig->m_window.getSize().x;
		gameConfig->minScrH = gameConfig->m_window.getSize().y;
		gameConfig->scrW = gameConfig->fullScrW;
		gameConfig->scrH = gameConfig->fullScrH;
		if (!gameConfig->wasFullScreen || firstStart)
		{
			if (gameConfig->transparent)
			{
				gameConfig->scrW = gameConfig->fullScrW + 1;
				gameConfig->scrH = gameConfig->fullScrH + 1;
				gameConfig->m_window.setSize({ (sf::Uint16)gameConfig->scrW, (sf::Uint16)gameConfig->scrH });
				gameConfig->m_window.setPosition({ 0,0 });
				sf::View v = gameConfig->m_window.getView();
				v.setSize({ gameConfig->scrW, gameConfig->scrH });
				v.setCenter({ gameConfig->scrW / 2, gameConfig->scrH / 2 });
				gameConfig->m_window.setView(v);
			}
			else
				gameConfig->m_window.create(sf::VideoMode(gameConfig->fullScrW, gameConfig->fullScrH), "VisualiStar", sf::Style::Fullscreen);
		}
	}
	else
	{
		gameConfig->scrW = gameConfig->minScrW;
		gameConfig->scrH = gameConfig->minScrH;
		if (gameConfig->wasFullScreen || firstStart)
			gameConfig->m_window.create(sf::VideoMode(gameConfig->scrW, gameConfig->scrH), "VisualiStar", 0);
		else
		{
			gameConfig->m_window.setSize({ (sf::Uint16)gameConfig->scrW, (sf::Uint16)gameConfig->scrH });
			sf::View v = gameConfig->m_window.getView();
			v.setSize({ gameConfig->scrW, gameConfig->scrH });
			v.setCenter({ gameConfig->scrW / 2, gameConfig->scrH / 2 });
			gameConfig->m_window.setView(v);
		}

	}

	gameConfig->m_RT.create(gameConfig->scrW, gameConfig->scrH);

	gameConfig->m_currentWindow = &gameConfig->m_window;
	gameConfig->m_window.setIcon(gameConfig->ico.getSize().x, gameConfig->ico.getSize().y, gameConfig->ico.getPixelsPtr());

	gameConfig->topLeftBox.setPosition(0, 0);
	gameConfig->topLeftBox.setSize({ 20,20 });
	gameConfig->topLeftBox.setFillColor({ 255,255,255,100 });
	gameConfig->bottomRightBox = gameConfig->topLeftBox;
	gameConfig->bottomRightBox.setPosition({ gameConfig->scrW - 20, gameConfig->scrH - 20 });
	gameConfig->resizeBox.setSize({ gameConfig->scrW, gameConfig->scrH });
	gameConfig->resizeBox.setOutlineThickness(1);
	gameConfig->resizeBox.setFillColor({ 0,0,0,0 });

	gameConfig->wasFullScreen = gameConfig->isFullScreen;


	gameConfig->m_window.setVerticalSyncEnabled(true);

	gameConfig->m_window.setFramerateLimit(60);

	if (gameConfig->transparent)
	{
		MARGINS margins;
		margins.cxLeftWidth = -1;

		SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
		DwmExtendFrameIntoClientArea(gameConfig->m_window.getSystemHandle(), &margins);
	}
}

void swapFullScreen()
{
	gameConfig->isFullScreen = !gameConfig->isFullScreen;
	initWindow();
	gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
	gameConfig->m_currentVis->reloadShader();
}

void menu()
{
	ImGui::SFML::Update(gameConfig->m_window, gameConfig->m_timer.restart());

	auto& style = ImGui::GetStyle();
	style.FrameRounding = 5;
	style.WindowTitleAlign = style.ButtonTextAlign;

	ImVec4 colA = gameConfig->gradCol1;
	ImVec4 colB = gameConfig->gradCol2;
	ImVec4 halfGrad(mean(colA.x, colB.x), mean(colA.y, colB.y), mean(colA.z, colB.z), 1.f );

	ImVec4 col_dark(halfGrad.x*0.3f, halfGrad.y*0.3f, halfGrad.z*0.3f, 1.f);
	ImVec4 col_med(halfGrad.x*0.5f, halfGrad.y*0.5f, halfGrad.z*0.5f, 1.f);
	ImVec4 col_light(halfGrad.x*0.8f, halfGrad.y*0.8f, halfGrad.z*0.8f, 1.f);
	ImVec4 col_light2(halfGrad);
	ImVec4 col_light2a(mean(halfGrad.x, 0.6f), mean(halfGrad.y, 0.6f), mean(halfGrad.z, 0.6f), 1.f);
	ImVec4 col_light3(mean(halfGrad.x,1.f), mean(halfGrad.y, 1.f), mean(halfGrad.z, 1.f), 1.f);

	style.Colors[ImGuiCol_WindowBg] = { 0.05f,0.05f,0.05f, 1.0f };
	style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Header] = col_med;
	style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_HeaderActive] = col_light2;
	style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_HeaderHovered] = col_light;
	style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_TitleBgCollapsed] = col_dark;
	style.WindowTitleAlign = { 0.5f, 0.5f };
	style.AntiAliasedLines = true;

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSize({ 400, -1 });

	ImVec4 greyoutCol(0.3, 0.3, 0.3, 1.0);
	ImVec4 normalTextCol(col_light3);

	ImGui::Begin("VisualiStar", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	ImDrawList* dList = ImGui::GetWindowDrawList();

	//	FULLSCREEN
	if (ImGui::Button(gameConfig->isFullScreen? "Exit Fullscreen (F11)" : "Fullscreen (F11)", { -1,20 }))
	{
		swapFullScreen();
	}
	ImGui::PushStyleColor(ImGuiCol_Text, normalTextCol);
	if (ImGui::Checkbox("Transparent", &gameConfig->transparent))
	{
		if (gameConfig->transparent)
		{
			if (gameConfig->isFullScreen)
			{
				gameConfig->scrW = gameConfig->fullScrW + 1;
				gameConfig->scrH = gameConfig->fullScrH + 1;
				gameConfig->m_window.create(sf::VideoMode(gameConfig->scrW, gameConfig->scrH), "VisualiStar", 0);
				gameConfig->m_window.setIcon((int)gameConfig->ico.getSize().x, (int)gameConfig->ico.getSize().y, gameConfig->ico.getPixelsPtr());
				gameConfig->m_window.setSize({ (sf::Uint16)gameConfig->scrW, (sf::Uint16)gameConfig->scrH });
				gameConfig->m_window.setPosition({ 0,0 });
				sf::View v = gameConfig->m_window.getView();
				v.setSize({ gameConfig->scrW, gameConfig->scrH });
				v.setCenter({ gameConfig->scrW / 2, gameConfig->scrH / 2 });
				gameConfig->m_window.setView(v);
			}

			MARGINS margins;
			margins.cxLeftWidth = -1;
			//SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED);
			//EnableWindow(gameConfig->m_window.getSystemHandle(), false);

			DwmExtendFrameIntoClientArea(gameConfig->m_window.getSystemHandle(), &margins);

			//gameConfig->menuShowing = false;
		}
		else
		{
			SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, 0);
			EnableWindow(gameConfig->m_window.getSystemHandle(), true);
		}
		if (!gameConfig->alwaysOnTop)
		{
			if (gameConfig->transparent)
			{
				HWND hwnd = gameConfig->m_window.getSystemHandle();
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			else
			{
				HWND hwnd = gameConfig->m_window.getSystemHandle();
				SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
		}
	}
	if (gameConfig->transparent)
	{
		ImGui::SameLine(227);
		ImGui::PushItemWidth(60);
		ImGui::DragFloat("Opacity", &gameConfig->minOpacity, 0.005f, 0.1f, 0.8f, "%.2f");
		ImGui::PopItemWidth();
	}

	if (ImGui::Checkbox("Always On Top", &gameConfig->alwaysOnTop))
	{
		if (gameConfig->alwaysOnTop)
		{
			HWND hwnd = gameConfig->m_window.getSystemHandle();
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		else
		{
			HWND hwnd = gameConfig->m_window.getSystemHandle();
			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}

	ImGui::PopStyleColor();

	if (ImGui::Button("Help", { -1,20 }))
	{
		float h = ImGui::GetWindowHeight();
		ImGui::SetNextWindowSize({ 400, h });
		ImGui::OpenPopup("Help");
	}
	ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.1f,0.1f,0.1f,1.f });

	ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSize({ 400,-1 });
	//ImGui::SetNextWindowSizeConstraints({ 400, 400 }, { -1,-1 });
	if (ImGui::BeginPopup("Help", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, col_light3);
		ImGui::TextColored(col_light2a, "Close Menu:");				ImGui::SameLine(160); ImGui::TextWrapped("Closes the main menu. ");
		ImGui::TextColored(col_light2a, "Fullscreen:");				ImGui::SameLine(160); ImGui::TextWrapped("Toggles fullscreen.");
		ImGui::TextColored(col_light2a, "Transparent:");			ImGui::SameLine(160); ImGui::TextWrapped("Turns VisualiStar into a transparent overlay so that you can use other programs beneath it.");
		ImGui::TextColored(col_light2a, "Opacity:");				ImGui::SameLine(160); ImGui::TextWrapped("Choose the opacity level for the transparent window.");
		ImGui::TextColored(col_light2a, "Always On Top:");			ImGui::SameLine(160); ImGui::TextWrapped("Keeps the visualiser above all other windows on your screen.");
		ImGui::TextColored(col_light2a, "Choose Visualisers:");		ImGui::SameLine(160); ImGui::TextWrapped("The tickbox beside each name lets you include/exclude certain visualisers from the Cycle. \nThe play button beside each name will instantly switch to that visualiser.");
		ImGui::TextColored(col_light2a, "Cycle:");					ImGui::SameLine(160); ImGui::TextWrapped("Changes the visualiser periodically. You can set the delay when the box is ticked.");
		ImGui::TextColored(col_light2a, "Colours:");				ImGui::SameLine(160); ImGui::TextWrapped("Change what colours the visualisers will use.");
		ImGui::TextColored(col_light2a, "Audio Input:");			ImGui::SameLine(160); ImGui::TextWrapped("Lets you select which audio device affects the visuals.");
		ImGui::TextColored(col_light2a, "Exit VisualiStar:");		ImGui::SameLine(160); ImGui::TextWrapped("Closes the program.");
		ImGui::NewLine();
		ImGui::TextColored(col_light2a, "Window Controls:");
		ImGui::TextWrapped("Drag from the top-left or bottom-right corner to resize the window.\nDrag with the middle mouse button to move the whole window.");
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, { 0.3f,0.3f,0.3f,1.f });
		ImGui::TextWrapped("PortAudio (c) 1999-2006 Ross Bencina and Phil Burk");
		ImGui::TextWrapped("SFML (c) 2007-2019 Laurent Gomila");
		ImGui::TextWrapped("Dear ImGui (c) 2014-2019 Omar Cornut");
		ImGui::NewLine();

		ImGui::TextWrapped("VisualiStar (c) 2018-2019 Thomas Rule");
		ImGui::PopStyleColor();
		ImGui::Separator();
		if (ImGui::Button("OK", { -1,20 }))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();

	//	VISUALISER LIST
	ImGui::NewLine();

	ImGui::Columns(2, "visualisers/options", false);
	ImGui::SetColumnWidth(0, 220);

	ImGui::TextColored(normalTextCol, "Choose Visualisers");
	ImGui::TextColored({ 0.5, 0.5, 0.5, 1.0 }, "In Cycle  Play   Name");
	ImGui::BeginChild("Choose Visualiser", { -1, 150 });
	for (int v = 0; v < gameConfig->m_visualisers.size(); v++)
	{
		auto& vis = gameConfig->m_visualisers[v];
		
		ImGui::SetCursorPosX(18);
		ImGui::PushID((vis.id + "tick").c_str());
		ImGui::Checkbox("", &vis.inCycle);
		ImGui::PopID();
		ImGui::SameLine(74);

		ImGui::PushID((vis.id + "playbtn").c_str());
		if (ImGui::Button(">", { 20, 20 }))
		{
			vis.vis->init(&gameConfig->m_window, &gameConfig->m_RT);
			vis.vis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
			vis.vis->reloadShader();
			gameConfig->m_currentVis = vis.vis.get();
			gameConfig->visIdx = v;

			ImGui::CloseCurrentPopup();
		}
		ImGui::PopID();
		ImGui::SameLine(119);
		ImGui::TextColored(gameConfig->visIdx == v ? col_light3 : col_light2, vis.id.c_str());
	}
	ImGui::EndChild();

	ImGui::NextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, col_light3);
	ImGui::Checkbox("Cycle", &gameConfig->cycleVis);
	ImGui::PopStyleColor();

	ImGui::TextColored(gameConfig->cycleVis ? normalTextCol : greyoutCol, "Time: %ds", gameConfig->cycleTime);
	ImGui::SameLine();
	if (ImGui::SmallButton("-"))
	{
		if (gameConfig->cycleVis && gameConfig->cycleTime > 5)
			gameConfig->cycleTime -= 5;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("+"))
	{
		if (gameConfig->cycleVis && gameConfig->cycleTime < 300)
			gameConfig->cycleTime += 5;
	}

	int countdown = gameConfig->cycleTime - (int)gameConfig->visTimer.getElapsedTime().asSeconds();
	if (countdown < 0) countdown = 0;

	ImGui::TextColored(col_light, gameConfig->cycleVis ? "Next change - %ds" : "", countdown);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
	ImGui::TextColored(normalTextCol, "Colours:"); ImGui::SameLine();
	if (gameConfig->gradient)
	{
		if (ImGui::Button("Gradient"))
		{
			gameConfig->gradient = false;
			gameConfig->m_currentVis->reloadShader();
		}

		//ImGui::SameLine();
		if (ImGui::ColorButton("Colour 1", gameConfig->gradCol1, 0, {10,20}))
		{
			gameConfig->editingColour = &gameConfig->gradCol1;
			ImGui::OpenPopup("ColPopup");
		}
		ImGui::SameLine(0, 0);

		auto pos1 = ImGui::GetCursorScreenPos();
		auto pos2 = ImVec2(pos1.x + 130, pos1.y + 20);
		ImColor col1(gameConfig->gradCol1);
		ImColor col2(gameConfig->gradCol2);
		dList->AddRectFilledMultiColor(pos1, pos2, col1, col2, col2, col1);
		ImGui::SameLine(148);

		if (ImGui::ColorButton("Colour 2", gameConfig->gradCol2, 0, { 10,20 }))
		{
			gameConfig->editingColour = &gameConfig->gradCol2;
			ImGui::OpenPopup("ColPopup");
		}

		if (ImGui::BeginPopupContextItem("ColPopup"))
		{
			float col[3] = { (float)gameConfig->editingColour->r / 255.f,(float)gameConfig->editingColour->g / 255.f,(float)gameConfig->editingColour->b / 255.f };
			if (ImGui::ColorPicker3("Edit Colour", col))
			{
				gameConfig->editingColour->r = col[0] * 255;
				gameConfig->editingColour->g = col[1] * 255;
				gameConfig->editingColour->b = col[2] * 255;
			}

			if (ImGui::Button("Done"))
			{
				gameConfig->m_currentVis->reloadShader();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		
		ImGui::NewLine();

	}
	else
	{
		if (ImGui::Button("Random"))
		{
			gameConfig->gradient = true;
			gameConfig->m_currentVis->reloadShader();
		}
	}


	//	INPUT LIST
	ImGui::SetCursorPosY(280);
	ImGui::TextColored(normalTextCol, "Audio Input");

	if (gameConfig->firstMenu)
	{
		gameConfig->nDevices = Pa_GetDeviceCount();
		gameConfig->deviceList.clear();
		for (int dI = 0; dI < gameConfig->nDevices; dI++)
		{
			auto info = Pa_GetDeviceInfo(dI);
			if (info->hostApi == 0 && info->maxInputChannels > 0)
				gameConfig->deviceList.push_back({ info->name, dI });
		}
	}

	//ImGui::TextColored({ 0.5, 0.5, 0.5, 1.0 }, "Current:\n  %s", gameConfig->deviceList[gameConfig->devIdx].first.c_str());
	ImGui::PushID("AudImpCombo");
	ImGui::PushItemWidth(150);
	if (ImGui::BeginCombo("", gameConfig->deviceList[gameConfig->devIdx].first.c_str()))
	{
		for (auto& dev : gameConfig->deviceList)
		{
			if (ImGui::Button(dev.first.c_str(), { -1, 20 }))
			{
				Pa_StopStream(gameConfig->AudioStr);
				Pa_CloseStream(gameConfig->AudioStr);

				auto info = Pa_GetDeviceInfo(dev.second);
				float sRate;

				gameConfig->devIdx = dev.second;
				gameConfig->params.device = gameConfig->devIdx;
				gameConfig->params.channelCount = info->maxInputChannels;
				gameConfig->params.suggestedLatency = info->defaultLowInputLatency;
				gameConfig->params.hostApiSpecificStreamInfo = nullptr;
				sRate = info->defaultSampleRate;

				Pa_OpenStream(&gameConfig->AudioStr, &gameConfig->params, nullptr, sRate, FRAMES_PER_BUFFER, paClipOff, recordCallback, gameConfig->streamData);
				Pa_StartStream(gameConfig->AudioStr);

				gameConfig->frameMax = 0.05;
				gameConfig->frameHi = 0;
				gameConfig->runningAverage = 0;

				gameConfig->bassMax = 0.05;
				gameConfig->bassHi = 0;
				gameConfig->bassAverage = 0;

				gameConfig->trebleMax = 0.05;
				gameConfig->trebleHi = 0;
				gameConfig->trebleAverage = 0;


				gameConfig->m_currentVis->init(&gameConfig->m_window, &gameConfig->m_RT);
				gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
				gameConfig->m_currentVis->reloadShader();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
	ImGui::PopItemWidth();

	ImGui::Columns();

	ImGui::Separator();

	if (ImGui::Button("Close Menu (Esc)", { -1,20 }))
	{
		gameConfig->menuShowing = false;
		if (gameConfig->transparent)
			SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED);
	}

	//	EXIT
	ImGui::PushStyleColor(ImGuiCol_Button, col_dark);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f + 0.7f * gameConfig->bassHi/gameConfig->bassMax, 0.f, 0.f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.3f, 0.f, 0.f, 1.0f });
	if (ImGui::Button("Exit VisualiStar", { -1, 20 }))
	{
		gameConfig->m_window.close();
	}
	ImGui::PopStyleColor(3);

	ImGui::End();
	ImGui::EndFrame();
	ImGui::SFML::Render(gameConfig->m_window);

	gameConfig->firstMenu = false;
}

void handleEvents()
{
	if (gameConfig->m_currentWindow->hasFocus() && gameConfig->isFullScreen && !gameConfig->menuShowing && !gameConfig->transparent)
	{
		gameConfig->m_currentWindow->setMouseCursorVisible(false);
	}
	else
	{
		gameConfig->m_currentWindow->setMouseCursorVisible(true);
	}

	if (gameConfig->cornerGrabbed.first || gameConfig->cornerGrabbed.second)
	{
		gameConfig->m_currentWindow->requestFocus();
	}

	sf::Event evt;
	while (gameConfig->m_currentWindow->pollEvent(evt))
	{
		if (evt.type == evt.Closed)
		{
			//close the application
			gameConfig->m_currentWindow->close();
			break;
		}
		else if (evt.type == evt.GainedFocus && gameConfig->transparent)
		{
			//gaining focus when transparent makes it non-transparent again and shows the menu
			gameConfig->transparent = false;
			gameConfig->m_currentWindow->setMouseCursorVisible(true);
			SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, 0);
			EnableWindow(gameConfig->m_window.getSystemHandle(), true);

			if (!gameConfig->alwaysOnTop)
			{
				HWND hwnd = gameConfig->m_window.getSystemHandle();
				SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}

			gameConfig->menuShowing = true;
		}
		else if (evt.type == evt.MouseButtonPressed && sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			//check if user clicked in the corners for window resize
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !gameConfig->isFullScreen)
			{
				auto pos = sf::Mouse::getPosition(gameConfig->m_window);
				if (pos.x < 20 && pos.y < 20)
					gameConfig->cornerGrabbed = { true, false };

				if (pos.x > gameConfig->scrW - 20 && pos.y > gameConfig->scrH - 20)
					gameConfig->cornerGrabbed = { false, true };
			}
		}
		else if ((evt.type == evt.KeyPressed && evt.key.code == sf::Keyboard::Escape) || (evt.type == evt.MouseButtonPressed && sf::Mouse::isButtonPressed(sf::Mouse::Right)))
		{
			//toggle the menu visibility
			gameConfig->menuShowing = !gameConfig->menuShowing;
			if (gameConfig->menuShowing)
				gameConfig->firstMenu = true;
			else if (gameConfig->transparent)
				SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED);

			break;
		}
		else if (evt.type == evt.KeyPressed && evt.key.code == sf::Keyboard::F11 && evt.key.control == false)
		{
			//toggle fullscreen
			swapFullScreen();
		}
		else if (evt.type == evt.MouseButtonReleased)
		{
			//set the window if it was being resized
			if (gameConfig->cornerGrabbed.first || gameConfig->cornerGrabbed.second)
			{
				auto windowPos = gameConfig->m_window.getPosition();
				windowPos += sf::Vector2i(gameConfig->resizeBox.getPosition());
				gameConfig->m_window.setPosition(windowPos);
				gameConfig->minScrH = gameConfig->resizeBox.getGlobalBounds().height;
				gameConfig->minScrW = gameConfig->resizeBox.getGlobalBounds().width;
				gameConfig->cornerGrabbed = { false, false };

				initWindow();
				gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
				gameConfig->m_currentVis->reloadShader();
			}
			gameConfig->lastMiddleClickPosition = sf::Vector2i(-1, -1);
		}
		else if (evt.type == evt.MouseMoved)
		{
			auto pos = sf::Vector2f(sf::Mouse::getPosition(gameConfig->m_window));
			if (gameConfig->cornerGrabbed.first)
			{
				gameConfig->topLeftBox.setPosition(pos);
				gameConfig->resizeBox.setPosition(pos);
				gameConfig->resizeBox.setSize({ gameConfig->scrW - pos.x, gameConfig->scrH - pos.y });
			}
			else if (gameConfig->cornerGrabbed.second)
			{
				gameConfig->bottomRightBox.setPosition({ pos.x - 20, pos.y - 20 });
				gameConfig->resizeBox.setPosition(0, 0);
				gameConfig->resizeBox.setSize({ pos.x, pos.y });
			}
			else if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
			{
				if (gameConfig->lastMiddleClickPosition == sf::Vector2i(-1, -1))
					gameConfig->lastMiddleClickPosition = sf::Mouse::getPosition(gameConfig->m_window);
				auto mousePos = sf::Mouse::getPosition();
				auto windowPos = mousePos - gameConfig->lastMiddleClickPosition;
				gameConfig->m_window.setPosition(windowPos);
			}
		}

		if (gameConfig->menuShowing)
			ImGui::SFML::ProcessEvent(evt);
	}
}

void render()
{
	gameConfig->m_currentVis->render(gameConfig->frame, gameConfig->runningAverage, gameConfig->frameMax);

	if (gameConfig->menuShowing)
	{
		if (!gameConfig->isFullScreen)
		{
			gameConfig->m_window.draw(gameConfig->topLeftBox);
			gameConfig->m_window.draw(gameConfig->bottomRightBox);
		}
		menu();
	}

#ifdef _DEBUG
	//draw debug audio bars
	{
		std::lock_guard<std::mutex> guard(gameConfig->FreqDataMutex);

		auto frames = gameConfig->FrequencyData;
		float barW = gameConfig->scrW / (frames.size() / 2);

		for (int bar = 0; bar < frames.size() / 2; bar++)
		{
			float height = (frames[bar] / gameConfig->bassMax)*gameConfig->scrH;
			gameConfig->bars[bar].setSize({ barW, height });
			gameConfig->bars[bar].setOrigin({ 0.f, height });
			gameConfig->bars[bar].setPosition({ barW*bar, gameConfig->scrH });
			gameConfig->m_window.draw(gameConfig->bars[bar]);
	}
}
#endif


	if (gameConfig->cornerGrabbed.first)
	{
		gameConfig->m_window.draw(gameConfig->topLeftBox);
		gameConfig->m_window.draw(gameConfig->resizeBox);
	}
	else if (gameConfig->cornerGrabbed.second)
	{
		gameConfig->m_window.draw(gameConfig->bottomRightBox);
		gameConfig->m_window.draw(gameConfig->resizeBox);
	}

	gameConfig->m_window.display();
}


int main()
{
	PaError             err = paNoError;

	std::srand(time(0));

	gameConfig = new Config();

	getWindowSizes();

	gameConfig->ico.loadFromFile("icon1.png");

	initWindow(true);
	ImGui::SFML::Init(gameConfig->m_window);

	//setup visualisers
	gameConfig->m_visualisers.resize(6);

	gameConfig->m_visualisers[0] = { "StarFall", true, std::make_shared<StarFallVis>() };
	gameConfig->m_visualisers[0].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_visualisers[1] = { "Tesseract", true, std::make_shared<TesseractVis>() };
	gameConfig->m_visualisers[1].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_visualisers[2] = { "Vortex", true, std::make_shared<VortexVis>() };
	gameConfig->m_visualisers[2].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_visualisers[3] = { "Doorway", true, std::make_shared<DoorwayVis>() };
	gameConfig->m_visualisers[3].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_visualisers[4] = { "Archangel", true, std::make_shared<AngelVis>() };
	gameConfig->m_visualisers[4].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_visualisers[5] = { "Between", true, std::make_shared<BetweenVis>() };
	gameConfig->m_visualisers[5].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

	gameConfig->m_currentVis = gameConfig->m_visualisers[0].vis.get();

	gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
	gameConfig->m_currentVis->reloadShader();

	//setup debug bars
	gameConfig->frames.resize(FRAMES_PER_BUFFER * 3);
	gameConfig->bars.resize(FRAMES_PER_BUFFER / 2);
	float barW = gameConfig->scrW / (FRAMES_PER_BUFFER / 2);
	for (int b = 0; b < gameConfig->bars.size(); b++)
	{
		gameConfig->bars[b].setPosition((float)b*barW, 0);
		gameConfig->bars[b].setSize({ barW, 0.f });
		gameConfig->bars[b].setFillColor({ 255, 255, 255, 50 });
	}

	//initialise PortAudio
	err = Pa_Initialize();
	gameConfig->nDevices = Pa_GetDeviceCount();

	gameConfig->params.sampleFormat = PA_SAMPLE_TYPE;
	double sRate;

	auto defOutInf = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice());

	//find an audio input
	for (PaDeviceIndex dI = 0; dI < gameConfig->nDevices; dI++)
	{
		auto info = Pa_GetDeviceInfo(dI);
		std::string name = info->name;
		if (name.find("Stereo Mix") != std::string::npos || name.find("Wave Out") != std::string::npos)
		{
			gameConfig->devIdx = dI;
			gameConfig->params.device = gameConfig->devIdx;
			gameConfig->params.channelCount = info->maxInputChannels;
			gameConfig->params.suggestedLatency = info->defaultLowInputLatency;
			gameConfig->params.hostApiSpecificStreamInfo = nullptr;
			sRate = info->defaultSampleRate;
			break;
		}
	}

	err = Pa_OpenStream(&gameConfig->AudioStr, &gameConfig->params, nullptr, sRate, FRAMES_PER_BUFFER, paClipOff, recordCallback, gameConfig->streamData);
	auto errorMsg = Pa_GetErrorText(err);
	err = Pa_StartStream(gameConfig->AudioStr);
	errorMsg = Pa_GetErrorText(err);

	//request focus and start the game loop
	gameConfig->m_currentWindow->requestFocus();

	gameConfig->visTimer.restart();

	while (gameConfig->m_currentWindow->isOpen())
	{
		//update audio data for this frame
		if (gameConfig->frameHi != 0)
		{
			gameConfig->frame = (gameConfig->frameHi);
			if (gameConfig->frame < 0)
				gameConfig->frame *= -1;
			gameConfig->runningAverage -= gameConfig->runningAverage / 30;
			gameConfig->runningAverage += gameConfig->frame / 30;
			if (gameConfig->frame > gameConfig->frameMax)
				gameConfig->frameMax = gameConfig->frame;

			gameConfig->bassAverage -= gameConfig->bassAverage / 30;
			gameConfig->bassAverage += gameConfig->bassHi / 30;
			if (gameConfig->bassHi > gameConfig->bassMax)
				gameConfig->bassMax = gameConfig->bassHi;

			gameConfig->trebleAverage -= gameConfig->trebleAverage / 30;
			gameConfig->trebleAverage += gameConfig->trebleHi / 30;
			if (gameConfig->trebleHi > gameConfig->trebleMax)
				gameConfig->trebleMax = gameConfig->trebleHi;
		}

		handleEvents();
		if (!gameConfig->m_currentWindow->isOpen())
			break;

		render();

		//if the cycle timer ran out and we're on a significant beat in the music, go to the next visualiser
		if (gameConfig->cycleVis && gameConfig->visTimer.getElapsedTime().asSeconds() > gameConfig->cycleTime && gameConfig->frameHi > 1.5f*gameConfig->runningAverage)
		{
			gameConfig->visTimer.restart();
			do
			{
				gameConfig->visIdx++;
				if (gameConfig->visIdx > gameConfig->m_visualisers.size() - 1)
					gameConfig->visIdx = 0;
			} while (!gameConfig->m_visualisers[gameConfig->visIdx].inCycle);

			gameConfig->m_currentVis = gameConfig->m_visualisers[gameConfig->visIdx].vis.get();
			gameConfig->m_currentVis->init(&gameConfig->m_window, &gameConfig->m_RT);
			gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
			gameConfig->m_currentVis->reloadShader();
		}

		gameConfig->frameHi = 0;
		gameConfig->bassHi = 0;
		gameConfig->trebleHi = 0;

		sf::sleep(sf::milliseconds(10));
	}

	Pa_StopStream(gameConfig->AudioStr);
	Pa_CloseStream(gameConfig->AudioStr);
	Pa_Terminate();

	ImGui::SFML::Shutdown();

	if (gameConfig->m_currentWindow->isOpen())
		gameConfig->m_currentWindow->close();


	delete gameConfig;
	return 0;

}
