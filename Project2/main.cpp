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
#include "SingularityVis.h"

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

	//Erase old frames, leave enough in the vector for one checkSize
	if (gameConfig->frames.size() > checkSize)
		gameConfig->frames.erase(gameConfig->frames.begin(), gameConfig->frames.begin() + (gameConfig->frames.size()-checkSize));

	SAMPLE *rptr = (SAMPLE*)inputBuffer;

	int s = 0;

	while (s < checkSize)
	{
		SAMPLE splLeft = (*rptr++);
		SAMPLE splRight = splLeft;
		if (numChannels == 2)
			splRight = (*rptr++);

		SAMPLE spl = (splLeft + splRight) / 2.f;

		spl = spl*spl;

		{ //lock for frequency data
			std::lock_guard<std::mutex> guard(gameConfig->FreqDataMutex);

			gameConfig->frames.push_back(fabs(spl));
		}

		s++;
	}

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
			auto pos = gameConfig->m_window.getPosition();
			gameConfig->scrX = pos.x;
			gameConfig->scrY = pos.y;
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
		{
			gameConfig->m_window.create(sf::VideoMode(gameConfig->scrW, gameConfig->scrH), "VisualiStar", 0);
			gameConfig->m_window.setPosition({ gameConfig->scrX, gameConfig->scrY });
		}
		else
		{
			gameConfig->m_window.setSize({ (sf::Uint16)gameConfig->scrW, (sf::Uint16)gameConfig->scrH });
			sf::View v = gameConfig->m_window.getView();
			v.setSize({ gameConfig->scrW, gameConfig->scrH });
			v.setCenter({ gameConfig->scrW / 2, gameConfig->scrH / 2 });
			gameConfig->m_window.setView(v);

			auto pos = gameConfig->m_window.getPosition();
			if (gameConfig->scrX != pos.x || gameConfig->scrY != pos.y)
			{
				gameConfig->m_window.setPosition({ gameConfig->scrX, gameConfig->scrY });
			}
		}
	}

	gameConfig->m_RT.create(gameConfig->scrW, gameConfig->scrH);

	gameConfig->m_currentWindow = &gameConfig->m_window;
	

	gameConfig->topLeftBox.setPosition(0, 0);
	gameConfig->topLeftBox.setSize({ 20,20 });
	gameConfig->topLeftBox.setFillColor({ 255,255,255,100 });
	gameConfig->bottomRightBox = gameConfig->topLeftBox;
	gameConfig->bottomRightBox.setPosition({ gameConfig->scrW - 20, gameConfig->scrH - 20 });
	gameConfig->resizeBox.setSize({ gameConfig->scrW, gameConfig->scrH });
	gameConfig->resizeBox.setOutlineThickness(1);
	gameConfig->resizeBox.setFillColor({ 0,0,0,0 });

	gameConfig->wasFullScreen = gameConfig->isFullScreen;

	gameConfig->m_window.setVerticalSyncEnabled(gameConfig->enableVSync);

	gameConfig->m_window.setFramerateLimit(gameConfig->enableVSync? 60 : 200);

	if (gameConfig->transparent)
	{
		MARGINS margins;
		margins.cxLeftWidth = -1;

		SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
		DwmExtendFrameIntoClientArea(gameConfig->m_window.getSystemHandle(), &margins);
	}

	if (gameConfig->ico.getPixelsPtr())
	{
		gameConfig->m_window.setIcon(gameConfig->ico.getSize().x, gameConfig->ico.getSize().y, gameConfig->ico.getPixelsPtr());
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
	ImGui::SetNextWindowSize({ 480, -1 });

	ImVec4 greyoutCol(0.3, 0.3, 0.3, 1.0);
	ImVec4 normalTextCol(col_light3);

	ImGui::Begin("VisualiStar", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

	ImDrawList* dList = ImGui::GetWindowDrawList();

	if (ImGui::Button("Close Menu (Esc)", { -1,20 }))
	{
		gameConfig->menuShowing = false;
		if (gameConfig->transparent)
			SetWindowLong(gameConfig->m_window.getSystemHandle(), GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED);
	}

	//	FULLSCREEN
	if (ImGui::Button(gameConfig->isFullScreen? "Exit Fullscreen (F11)" : "Fullscreen (F11)", { -1,20 }))
	{
		swapFullScreen();
	}

		if (ImGui::Button("Help", { ImGui::GetWindowWidth()/2 - 10,20 }))
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
		ImGui::TextColored(col_light2a, "Show Advanced...:");		ImGui::SameLine(160); ImGui::TextWrapped("Reveals some advanced options.");
		ImGui::TextColored(col_light2a, "Choose Visualisers:");		ImGui::SameLine(160); ImGui::TextWrapped("The tickbox beside each name lets you include/exclude certain visualisers from the Cycle. \nThe play button beside each name will instantly switch to that visualiser.");
		ImGui::TextColored(col_light2a, "Cycle:");					ImGui::SameLine(160); ImGui::TextWrapped("Changes the visualiser periodically. You can set the delay when the box is ticked.");
		ImGui::TextColored(col_light2a, "Colours:");				ImGui::SameLine(160); ImGui::TextWrapped("Change what colours the visualisers will use.");
		ImGui::TextColored(col_light2a, "Audio Input:");			ImGui::SameLine(160); ImGui::TextWrapped("Lets you select which audio device affects the visuals.");
		ImGui::TextColored(col_light2a, "Presets:");				ImGui::SameLine(160); ImGui::TextWrapped("Save and load Presets for the visualiser and window settings.");
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

	ImGui::SameLine(); 
	if (ImGui::Button(gameConfig->advancedMenuShowing? "Hide Advanced"  : "Show Advanced...", { -1,20 }))
	{
		gameConfig->advancedMenuShowing = !gameConfig->advancedMenuShowing;
	}
	if (gameConfig->advancedMenuShowing)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, normalTextCol);
		float transChkBoxPos = ImGui::GetCursorPosY();
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
		ImGui::PushStyleColor(ImGuiCol_Text, col_light2a);
		ImGui::SameLine(140); ImGui::TextWrapped("Turns VisualiStar into a transparent overlay so that you can use other programs beneath it.");
		ImGui::PopStyleColor();

		if (gameConfig->transparent)
		{
			//ImGui::SameLine(227);
			ImGui::SetCursorPosY(transChkBoxPos + 22);
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
		ImGui::PushStyleColor(ImGuiCol_Text, col_light2a);
		ImGui::SameLine(140); ImGui::TextWrapped("Keeps the visualiser above all other windows on your screen.");
		ImGui::PopStyleColor();

		ImGui::Checkbox("Show FPS", &gameConfig->showFPS);
		ImGui::PushStyleColor(ImGuiCol_Text, col_light2a);
		ImGui::SameLine(140); ImGui::TextWrapped("Shows an FPS counter (when menu is inactive).");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("VSync", &gameConfig->enableVSync))
		{
			initWindow();
		}
		ImGui::PushStyleColor(ImGuiCol_Text, col_light2a);
		ImGui::SameLine(140); ImGui::TextWrapped("Enable/Disable Vertical Sync.");
		ImGui::PopStyleColor();

		ImGui::PopStyleColor();
	}

	ImGui::Separator();

	//	VISUALISER LIST

	ImGui::Columns(2, "visualisers/options", false);
	ImGui::SetColumnWidth(0, 260);

	ImGui::TextColored(normalTextCol, "Choose Visualisers");
	ImGui::TextColored({ 0.5, 0.5, 0.5, 1.0 }, "In Cycle  Play   Name");
	ImGui::BeginChild("Choose Visualiser", { -1, 180 });
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

	auto colPos = ImGui::GetCursorPosY();

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

	ImGui::NewLine();


	//	INPUT LIST
	ImGui::SetCursorPosY(colPos+72);
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
			bool active = gameConfig->devIdx == dev.second;
			if(ImGui::Selectable(dev.first.c_str(), &active))
			{
				Pa_StopStream(gameConfig->AudioStr);
				Pa_CloseStream(gameConfig->AudioStr);

				auto info = Pa_GetDeviceInfo(dev.second);
				float sRate;

				gameConfig->devIdx = dev.second;
				gameConfig->params.device = gameConfig->devIdx;
				gameConfig->params.channelCount = min(2, info->maxInputChannels);
				gameConfig->params.suggestedLatency = info->defaultLowInputLatency;
				gameConfig->params.hostApiSpecificStreamInfo = nullptr;// (PaHostApiInfo*)Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
				sRate = info->defaultSampleRate;

				Pa_OpenStream(&gameConfig->AudioStr, &gameConfig->params, nullptr, sRate, FRAMES_PER_BUFFER, paClipOff, recordCallback, gameConfig->streamData);
				Pa_StartStream(gameConfig->AudioStr);

				gameConfig->frameMax = gameConfig->cutoff;
				gameConfig->frameHi = 0;
				gameConfig->runningAverage = 0;

				gameConfig->bassMax = gameConfig->cutoff;
				gameConfig->bassHi = 0;
				gameConfig->bassAverage = 0;

				gameConfig->trebleMax = gameConfig->cutoff;
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

	ImGui::NewLine();

	//WIP Background image stuff here
/*
	if (ImGui::Button("Background Image", { -1,20 }))
	{
		ImGui::OpenPopup("Pick Background Image");
	}

	if (ImGui::BeginPopup("Pick Background Image"))
	{
		ImGui::SetWindowSize({ 400,-1 });

		if (!gameConfig->backgroundImageName.empty())
			ImGui::Text(gameConfig->backgroundImageName.c_str());

		if (ImGui::Button("Browse"))
		{
			OPENFILENAME ofn;
			TCHAR szFile[260] = { 0 };
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = gameConfig->m_currentWindow->getSystemHandle();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = LPCTSTR("Image Files\0*.png;*.jpg;*.tif;*.bmp\0\0");
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn) == TRUE)
			{
				std::string imagePath(ofn.lpstrFile);
				if (!gameConfig->backgroundImage.get())
				{
					gameConfig->backgroundImage = std::make_shared<sf::Image>();
				}

				if (gameConfig->backgroundImage->loadFromFile(imagePath))
				{
					gameConfig->backgroundImageName = imagePath;
					gameConfig->backgroundTexture.reset();
					gameConfig->backgroundTexture = std::make_shared<sf::Texture>();
					gameConfig->backgroundTexture->loadFromImage(*(gameConfig->backgroundImage.get()));
				}
				else
				{
					gameConfig->backgroundImage.reset();
				}
				gameConfig->visTimer.restart();
			}
		}

		if (ImGui::Button("Clear BG Image"))
		{
			gameConfig->backgroundImage.reset();
			gameConfig->backgroundTexture.reset();
			gameConfig->backgroundImageName = "";
		}

		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	*/

	ImGui::Columns();

	if (ImGui::Button("Presets", { -1,20 }))
	{
		ImGui::OpenPopup("Presets");
	}
	if(ImGui::BeginPopup("Presets", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::SetWindowSize({ 400,-1 });
		std::string prevName = "";
		if (gameConfig->m_presetNames.size()) prevName = gameConfig->m_presetNames[gameConfig->m_presetIdx];
		if (ImGui::BeginCombo("Presets", prevName.c_str()))
		{
			for (int p = 0; p < gameConfig->m_presetNames.size(); p++)
			{
				bool selected = gameConfig->m_presetIdx == p;
				if (ImGui::Selectable(gameConfig->m_presetNames[p].c_str(), &selected))
				{
					gameConfig->m_presetIdx = p;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load", { 100,20 }) && gameConfig->m_presetNames.size())
		{
			gameConfig->loadFromSettingsFile(gameConfig->m_presetNames[gameConfig->m_presetIdx]);
			gameConfig->m_settingsFileBoxName.clear();

			if (gameConfig->windowSettingsChanged)
			{
				initWindow();
				gameConfig->m_currentVis->resetPositions(gameConfig->scrW, gameConfig->scrH, gameConfig->ratio);
				gameConfig->m_currentVis->reloadShader();
				gameConfig->windowSettingsChanged = false;
			}
		}
		ImGui::Separator();
		ImGui::TextColored(normalTextCol, "Save Preset");
		ImGui::InputText("Name", gameConfig->m_settingsFileBoxName.data(), 30);
		ImGui::SameLine();
		if (ImGui::Button("x", { 20,20 }))
		{
			gameConfig->m_settingsFileBoxName.clear();
			gameConfig->m_settingsFileBoxName.resize(30);
		}
		ImGui::SameLine();
		if (ImGui::Button("Use Current", { -1,20 }) && gameConfig->m_presetNames.size())
		{
			gameConfig->m_settingsFileBoxName.clear();
			gameConfig->m_settingsFileBoxName.resize(30);
			int i = 0;
			for (auto& c : gameConfig->m_presetNames[gameConfig->m_presetIdx])
				gameConfig->m_settingsFileBoxName[i++] = c;
		}

		bool overwriting = false;
		for (auto& n : gameConfig->m_presetNames)
		{
			if (n == std::string(gameConfig->m_settingsFileBoxName.data()))
			{
				overwriting = true;
				break;
			}
		}

		std::string saveCheckBox = "Save";
		if (overwriting) saveCheckBox = "Update";

		ImGui::PushID("VisSave");
		ImGui::TextColored(normalTextCol, "Visualiser Settings");
		ImGui::SameLine(); ImGui::TextColored(col_light2a, "(Visualisers in cycle, colours)");

		if (ImGui::Checkbox(saveCheckBox.c_str(), &gameConfig->saveVisInfo))
		{
			if (gameConfig->saveVisInfo)
				gameConfig->clearVisInfo = false;
		}
		if (overwriting)
		{
			ImGui::SameLine();
			if (ImGui::Checkbox("Clear", &gameConfig->clearVisInfo))
			{
				if (gameConfig->clearVisInfo)
					gameConfig->saveVisInfo = false;
			}
		}
		ImGui::PopID();
		ImGui::PushID("WndSave");
		ImGui::TextColored(normalTextCol, "Window Settings");
		ImGui::SameLine(); ImGui::TextColored(col_light2a, "(Size, position)");

		if (ImGui::Checkbox(saveCheckBox.c_str(), &gameConfig->saveWindowInfo))
		{
			if (gameConfig->saveWindowInfo)
				gameConfig->clearWindowInfo = false;
		}
		if (overwriting)
		{
			ImGui::SameLine();
			if (ImGui::Checkbox("Clear", &gameConfig->clearWindowInfo))
			{
				if (gameConfig->clearWindowInfo)
					gameConfig->saveWindowInfo = false;
			}
		}
		ImGui::PopID();

		std::string saveName = "Save";
		if (overwriting) saveName = "Overwrite";

		std::string name(gameConfig->m_settingsFileBoxName.data());

		if ( (name != "") && ImGui::Button(saveName.c_str(), { -1,20 }))
		{
			if (std::find(gameConfig->m_presetNames.begin(), gameConfig->m_presetNames.end(), name) == gameConfig->m_presetNames.end())
			{
				gameConfig->m_presetNames.push_back(name);
				gameConfig->m_presetIdx = gameConfig->m_presetNames.size() - 1;
			}
			gameConfig->saveToSettingsFile(gameConfig->m_presetNames[gameConfig->m_presetIdx]);
			gameConfig->m_settingsFileBoxName.clear();
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();

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
				gameConfig->scrX = windowPos.x;
				gameConfig->scrY = windowPos.y;
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
				gameConfig->scrX = windowPos.x;
				gameConfig->scrY = windowPos.y;
				gameConfig->m_window.setPosition(windowPos);
			}
		}

		if (gameConfig->menuShowing)
			ImGui::SFML::ProcessEvent(evt);
	}
}

void render()
{
	gameConfig->m_currentVis->render(gameConfig->frame, gameConfig->runningAverage, gameConfig->frameMax, gameConfig->backgroundTexture.get());

	if (gameConfig->menuShowing)
	{
		if (!gameConfig->isFullScreen)
		{
			gameConfig->m_window.draw(gameConfig->topLeftBox);
			gameConfig->m_window.draw(gameConfig->bottomRightBox);
		}
		menu();
	}
	else if(gameConfig->showFPS)
	{
		auto dt = gameConfig->m_timer.getElapsedTime();
		ImGui::SFML::Update(gameConfig->m_window, gameConfig->m_timer.restart());

		ImGui::Begin("", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);

		ImGui::Text("FPS: %d", (int)(1.0 / dt.asSeconds()) );

		ImGui::End();
		ImGui::EndFrame();
		ImGui::SFML::Render(gameConfig->m_window);
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
	gameConfig->m_settingsFile = "presets.txt";
	gameConfig->loadPresetList();
	gameConfig->m_settingsFileBoxName.resize(30);

	initWindow(true);
	ImGui::SFML::Init(gameConfig->m_window);

	//setup visualisers
	gameConfig->m_visualisers.resize(7);

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

	gameConfig->m_visualisers[6] = { "Singularity", true, std::make_shared<SingularityVis>() };
	gameConfig->m_visualisers[6].vis->init(&gameConfig->m_window, &gameConfig->m_RT);

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
			gameConfig->params.channelCount = min(2, info->maxInputChannels);
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
	gameConfig->quietTimer.restart();


	////////////////////////////////////// MAIN LOOP /////////////////////////////////////
	while (gameConfig->m_currentWindow->isOpen())
	{

		//Do fourier transform
		{ //lock for frequency data
			std::lock_guard<std::mutex> guard(gameConfig->FreqDataMutex);

			gameConfig->FFTdata = std::vector<SAMPLE>(gameConfig->frames.begin(), gameConfig->frames.begin() + (FRAMES_PER_BUFFER * 2));
		} //end lock

		auto halfSize = gameConfig->FFTdata.size() / 2;

		four1(gameConfig->FFTdata.data(), halfSize);
		float factor = 50;
		gameConfig->FrequencyData.clear();

		for (int it = 0; it < halfSize; it++)
		{
			auto re = gameConfig->FFTdata[it];
			auto im = gameConfig->FFTdata[it + 1];
			auto magnitude = std::sqrt(re*re + im*im);

			//Compensations for the fact my FFT implementation is probably wrong
			float point = it / factor + 0.3;
			magnitude = magnitude*atan(point);
			if (it == 0) magnitude *= 0.7;

			//Store the magnitude
			gameConfig->FrequencyData.push_back(magnitude);

			//store data for bass and treble
			if (it < FRAMES_PER_BUFFER / 10 && magnitude > gameConfig->bassHi)
				gameConfig->bassHi = magnitude;

			if (it > FRAMES_PER_BUFFER / 4 && it < FRAMES_PER_BUFFER / 2 && magnitude > gameConfig->trebleHi)
				gameConfig->trebleHi = magnitude;

			if (magnitude > gameConfig->frameHi && it < FRAMES_PER_BUFFER / 2)
				gameConfig->frameHi = magnitude;
		}


		if (gameConfig->frameHi != 0.0)
		{
			//update audio data for this frame
			if (gameConfig->frameHi < 0) gameConfig->frameHi *= -1.0;
			gameConfig->frame = gameConfig->frameHi;

			gameConfig->runningAverage -= gameConfig->runningAverage / 30;
			gameConfig->runningAverage += gameConfig->frame / 30;
			if (gameConfig->frame > gameConfig->frameMax)
				gameConfig->frameMax = gameConfig->frame;

			if (gameConfig->bassHi < 0) gameConfig->bassHi *= -1.0;

			gameConfig->bassAverage -= gameConfig->bassAverage / 30;
			gameConfig->bassAverage += gameConfig->bassHi / 30;
			if (gameConfig->bassHi > gameConfig->bassMax)
				gameConfig->bassMax = gameConfig->bassHi;

			if (gameConfig->trebleHi < 0) gameConfig->trebleHi *= -1.0;

			gameConfig->trebleAverage -= gameConfig->trebleAverage / 30;
			gameConfig->trebleAverage += gameConfig->trebleHi / 30;
			if (gameConfig->trebleHi > gameConfig->trebleMax)
				gameConfig->trebleMax = gameConfig->trebleHi;
		}

		//As long as the music is loud enough the current max is good
		if (gameConfig->frame > gameConfig->cutoff * 2)
		{
			gameConfig->quietTimer.restart();
		}
		else if (gameConfig->quietTimer.getElapsedTime().asSeconds() > 1.5)
		{
			//if the quietTimer reaches 1.5s, reset the max

			gameConfig->frameMax = (std::max)(gameConfig->runningAverage, gameConfig->cutoff * 2);
			gameConfig->bassMax = (std::max)(gameConfig->bassAverage, gameConfig->cutoff * 2);
			gameConfig->trebleMax = (std::max)(gameConfig->trebleAverage, gameConfig->cutoff * 2);
			gameConfig->quietTimer.restart();
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

		sf::sleep(sf::milliseconds(8));
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
