#include "main.h"

SAMPFUNCS *SF = new SAMPFUNCS();
using json = nlohmann::json;

enum eTabs : int {
	TAB_MAIN = 0,
	TAB_PRESETS,
	TAB_ROLE
};

struct sResponse {
	bool bError;
	std::string sText;
};

struct sGeminiMessage {
	std::string sRequest;
	std::string sResponse;
};

struct sMessage {
	std::string sText;
	bool bIsTest;
};

struct sParsedMessage {
	std::string text = "";
	int id = -1;
	std::string nick = "";
};

bool bTargetAnswer = false;
int iTargetPlayerId = 0;
int iDelays[] = {1000, 2000};
bool bGenerating = false;
bool bActive = false;
bool bMenu = false;
bool bWasMenu = false;
int iActiveTab = TAB_MAIN;
int iSelectedPreset = 0;
std::string sGeminiToken;
std::string sProxyIpPort;
std::string sProxyLogin;
std::string sProxyPassword;
std::string sRole;
std::string sTestPrompt;
std::string sNewPreset;
std::vector<sMessage> messageOrder = {};
std::vector<std::string> presets = {};
std::mutex mutex;

void SendClientMessage(std::string text) {
	SF->getSAMP()->getChat()->AddChatMessage(0xFF18A3FF, std::string("[Gemini]: {FFFFFF}" + text).c_str());
}

void AddMessageJumpQ(const char* text, unsigned int time, unsigned short flag, bool bPreviousBrief)
{
	((void(__cdecl*)(const char*, unsigned int, unsigned short, bool))0x69F1E0)(text, time, flag, bPreviousBrief);
}

class CGemini {
private:
	std::vector<sGeminiMessage> history = {};
public:

	void ClearHistory() {
		history.clear();
	}

	sResponse Send(std::string message) {
		sResponse res = { false, "" };
		httplib::Client cli("https://generativelanguage.googleapis.com");
		if (sProxyIpPort.size() > 0) {
			cli.set_proxy(sProxyIpPort.substr(0, sProxyIpPort.find(":")), atoi(sProxyIpPort.substr(sProxyIpPort.find(":") + 1, sProxyIpPort.size()).c_str()));
			if (sProxyLogin.size() > 0) {
				cli.set_proxy_basic_auth(sProxyLogin, sProxyPassword);
			}
		}
		json data;
		data["safetySettings"] = json::array();
		//data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_SEXUAL"}, {"threshold", "BLOCK_NONE"} });
		//data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_DANGEROUS"}, {"threshold", "BLOCK_NONE"} });
		data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_SEXUALLY_EXPLICIT"}, {"threshold", "BLOCK_NONE"} });
		data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_HATE_SPEECH"}, {"threshold", "BLOCK_NONE"} });
		data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_HARASSMENT"}, {"threshold", "BLOCK_NONE"} });
		data["safetySettings"].push_back({ {"category", "HARM_CATEGORY_DANGEROUS_CONTENT"}, {"threshold", "BLOCK_NONE"} });

		if (sRole.size() > 0) {
			//data["system_instruction"]["role"] = sRole;
			data["system_instruction"]["parts"]["text"] = sRole;
		}

		data["contents"] = json::array();
		json contentData;
		json textData;
		for (auto& element : history) {
			contentData["role"] = "user";
			contentData["parts"] = json::array();
			textData["text"] = element.sRequest;
			contentData["parts"].push_back(textData);
			data["contents"].push_back(contentData);

			contentData["role"] = "model";
			contentData["parts"] = json::array();
			textData["text"] = element.sResponse;
			contentData["parts"].push_back(textData);
			data["contents"].push_back(contentData);
		}
		contentData["role"] = "user";
		contentData["parts"] = json::array();
		textData["text"] = message;
		contentData["parts"].push_back(textData);
		data["contents"].push_back(contentData);

		if (auto result = cli.Post("/v1beta/models/gemini-1.5-flash:generateContent?key=" + sGeminiToken, data.dump(), "application/json")) {
			//printf("%s\n", result->body.c_str());
			json responseData = json::parse(result->body);
			if (responseData.contains("candidates")) {
				res.sText = responseData["candidates"][0]["content"]["parts"][0]["text"];
				res.bError = false;
				history.push_back({ message, res.sText });
			}
			else if (responseData.contains("error")) {
				res.bError = true;
				res.sText = responseData["error"]["message"];
			}
			else if (responseData.contains("blockReason")) {
				res.bError = true;
				res.sText = responseData["blockReason"];
			}
			
		}
		else {
			//printf("%s\n", httplib::to_string(result.error()).c_str());
			res.bError = true;
			res.sText = httplib::to_string(result.error());
		}
		return res;
	}

	int GetHistorySize() {
		return history.size();
	}

};

CGemini gemini;

void GeminiWorker() {
	while (true) {
		Sleep(1);
		mutex.lock();
		if (messageOrder.size() > 0) {
			bGenerating = true;
			sMessage message = messageOrder[0];
			messageOrder.erase(messageOrder.begin());
			mutex.unlock();
			sResponse response = gemini.Send(message.sText);
			if (response.bError)
				SendClientMessage(EncodingUtil::ConvertUTF8ToCP1251("Ошибка! " + response.sText));
			else {
				Sleep(MathUtil::generateRandomInt(iDelays[0], iDelays[1]));
				if (message.bIsTest)
					SendClientMessage(EncodingUtil::ConvertUTF8ToCP1251("Ответ: " + response.sText));
				else {
					SF->getSAMP()->getPlayers()->localPlayerInfo.data->Say((char*)EncodingUtil::ConvertUTF8ToCP1251(response.sText).c_str());
				}
			}
			bGenerating = false;
		}
		else
			mutex.unlock();
	}
}

void SaveSettings() {
	json data;
	data["token"] = sGeminiToken;
	data["proxyIpPort"] = sProxyIpPort;
	data["proxyLogin"] = sProxyLogin;
	data["proxyPort"] = sProxyPassword;
	data["presets"] = presets;
	data["delays"]["min"] = iDelays[0];
	data["delays"]["max"] = iDelays[1];
	data["targetPlayer"]["enabled"] = bTargetAnswer;
	data["targetPlayer"]["id"] = iTargetPlayerId;
	data["role"] = sRole;
	std::ofstream f("gemini.json");
	f << std::setw(4) << data << std::endl;
	f.close();
}

void LoadSettings() {
	std::ifstream f("gemini.json");
	if (f.fail())
		return SaveSettings();
	json data = json::parse(f);
	sGeminiToken = data["token"];
	sProxyIpPort = data["proxyIpPort"];
	sProxyLogin = data["proxyLogin"];
	sProxyPassword = data["proxyPort"];
	if (data.contains("presets"))
		presets = data["presets"];
	if (data.contains("delays")) {
		iDelays[0] = data["delays"]["min"];
		iDelays[1] = data["delays"]["max"];
	}
	if (data.contains("targetPlayer")) {
		bTargetAnswer = data["targetPlayer"]["enabled"];
		iTargetPlayerId = data["targetPlayer"]["id"];
	}
	if (data.contains("role"))
		sRole = data["role"];
}

std::string escapeRegex(const std::string& str) {
	std::string escaped;
	for (char c : str) {
		switch (c) {
		case '.':
		case '^':
		case '$':
		case '*':
		case '+':
		case '?':
		case '{':
		case '}':
		case '(':
		case ')':
		case '[':
		case ']':
		case '|':
		case '\\':
			escaped += '\\';
			break;
		}
		escaped += c;
	}
	return escaped;
}

std::string replaceAll(const std::string& str, const std::string& pattern, const std::string& replacement) {
	std::regex re(pattern);
	return std::regex_replace(str, re, replacement);
}

sParsedMessage parseMessage(std::string input) {
	for (int i = 0; i < presets.size(); i++) {
		sParsedMessage parsedMessage;
		std::string patternStr = presets[i];

		patternStr = escapeRegex(patternStr);

		std::regex pattern("/[^/]*/");
		std::smatch regexMatch;
		std::vector<std::string> patternValues = {};

		std::string::const_iterator searchStart(patternStr.cbegin());
		while (regex_search(searchStart, patternStr.cend(), regexMatch, pattern)) {
			patternValues.push_back(regexMatch[0]);
			searchStart = regexMatch.suffix().first;
		}

		if (patternValues.size() > 0) {
			patternStr = replaceAll(patternStr, "/[^/]*/", "(.+)");
			pattern = std::regex(patternStr);

			std::smatch match;
			if (std::regex_search(input, match, pattern) && match.size() - 1 == patternValues.size()) {
				for (int i = 1; i < match.size(); i++) {
					std::string text = std::string(match[i]);
					std::string patternText = std::string(patternValues[i - 1]);
					if (patternText == "/id/") {
						int id = -1;
						try {
							id = std::stoi(text);
						}
						catch (...) {}
						parsedMessage.id = id;
					}
					if (patternText == "/nick/")
						parsedMessage.nick = text;
					if (patternText == "/message/")
						parsedMessage.text = text;
				}
				return parsedMessage;
			}
		}
	}
	return sParsedMessage();
}

void CALLBACK OnGeminiCommand(std::string args) {
	bMenu ^= 1;
}

bool CALLBACK Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{

	if (SUCCEEDED(SF->getRender()->BeginRender()))
	{
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (bMenu)
		{
			float imguiSize[] = { 400, 400 };
			ImGui::SetNextWindowSize(ImVec2(imguiSize[0], imguiSize[1]), ImGuiCond_FirstUseEver);
			ImGui::Begin("Gemini", &bMenu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
			{
				ImGui::BeginChild("##tabs", ImVec2(MathUtil::GetPercent(imguiSize[0], 96), MathUtil::GetPercent(imguiSize[1], 15)), true);
				std::vector<std::string> tabs = { "Главная", "Пресеты", "Роль"};
				float buttonWidth = MathUtil::GetPercent(imguiSize[0], 88) / tabs.size();
				for (int i = 0; i < tabs.size(); i++) {
					bool needPop = false;
					if (i == iActiveTab) {
						needPop = true;
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
					}
					if (ImGui::Button(tabs[i].c_str(), ImVec2(buttonWidth, MathUtil::GetPercent(imguiSize[1], 11))))
						iActiveTab = i;
					if (needPop)
						ImGui::PopStyleColor();
					ImGui::SameLine();
				}
				ImGui::EndChild();
				ImGui::BeginChild("##main", ImVec2(MathUtil::GetPercent(imguiSize[0], 96), MathUtil::GetPercent(imguiSize[1], 75)), true);

				switch (iActiveTab) {
				case TAB_MAIN:
					if (ImGui::CollapsingHeader("Токен, прокси")) {
						ImGui::InputText("Токен Gemini", &sGeminiToken, ImGuiInputTextFlags_Password);
						ImGui::Separator();
						ImGui::InputText("IP:Port прокси", &sProxyIpPort);
						ImGui::PushItemWidth(MathUtil::GetPercent(imguiSize[0], 30));
						ImGui::InputText("Логин прокси", &sProxyLogin);
						ImGui::InputText("Пароль прокси", &sProxyPassword, ImGuiInputTextFlags_Password);
						ImGui::PopItemWidth();
					}
					ImGui::NewLine();
					if (ImGui::Button(bActive ? "Отключить" : "Включить", ImVec2(MathUtil::GetPercent(imguiSize[0], 92), MathUtil::GetPercent(imguiSize[1], 10))))
						bActive ^= 1;
					ImGui::NewLine();
					ImGui::Text("Задержка перед ответом (мс):");
					ImGui::PushItemWidth(MathUtil::GetPercent(imguiSize[0], 30));
					ImGui::InputInt("Мин.##delay", &iDelays[0]);
					ImGui::SameLine();
					ImGui::InputInt("Макс.##delay", &iDelays[1]);
					ImGui::PopItemWidth();
					ImGui::Checkbox("Отвечать конкретному ID", &bTargetAnswer);
					if (bTargetAnswer)
						ImGui::InputInt("ID игрока", &iTargetPlayerId);
					break;
				case TAB_PRESETS:
					ImGui::Text("Пресеты (кликните 2 раза лкм чтобы удалить элемент)");
					ImGui::BeginListBox("##presets");
					for (int i = 0; i < presets.size(); i++) {
						bool bSelected = i == iSelectedPreset;
						if (ImGui::Selectable(presets[i].c_str(), &bSelected))
							iSelectedPreset = i;
						if (ImGui::IsItemFocused() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
							presets.erase(presets.begin() + i);
							iSelectedPreset = 0;
							break;
						}
					}
					ImGui::EndListBox();
					ImGui::TextWrapped("Новый пресет (/id/ - айди, /nick/ - ник, /message/ - сообщение, /.../ - любая произвольная строка");
					ImGui::InputText("##newPreset", &sNewPreset);
					ImGui::SameLine();
					if (ImGui::Button("Добавить")) {
						presets.push_back(sNewPreset);
						sNewPreset = "";
					}
					break;
				case TAB_ROLE:
					ImGui::Text("Количество сообщений: %d", gemini.GetHistorySize() * 2);
					if (ImGui::Button("Очистить историю сообщений"))
						gemini.ClearHistory();
					ImGui::Text("Роль");
					ImGui::InputText("##role", &sRole);
					ImGui::TextWrapped(sRole.c_str());
					ImGui::Text("Тестовое сообщение");
					ImGui::InputText("##testPrompt", &sTestPrompt);
					ImGui::SameLine();
					if (ImGui::Button("Отправить")) {
						mutex.lock();
						messageOrder.push_back({ sTestPrompt, true });
						mutex.unlock();
						sTestPrompt = "";
					}
					break;
				}

				ImGui::EndChild();
			}
			ImGui::End();
		}
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		SF->getRender()->EndRender();
	}
	return true;
}

HRESULT CALLBACK Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	return true;
}

bool CALLBACK WndProcHandler(HWND hwd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hwd, msg, wParam, lParam);
	static bool show_cursor = false;
	if (bMenu)
	{
		show_cursor = true;
		SF->getSAMP()->getMisc()->ToggleCursor(3);
		SF->getSAMP()->getInput()->DisableInput();
		if (msg == WM_KEYDOWN)
		{
			if (wParam == VK_ESCAPE)
				bMenu ^= true;
			return true;
		}
	}
	else
	{
		if (show_cursor)
		{
			SF->getSAMP()->getMisc()->ToggleCursor(0);
			show_cursor = false;
		}
	}
	return true;
}

bool CALLBACK OnIncomingRPC(stRakNetHookParams* params) {

	if (params->packetId == 93) {
		if (!bActive) return true;
		auto bs = params->bitStream;
		bs->IgnoreBits(32);
		uint32_t messageLength;
		bs->Read(messageLength);
		char* message = new char[messageLength];
		bs->Read(message, messageLength);
		//printf("parseMessage\n");
		sParsedMessage parsedMessage = parseMessage(std::string(message));
		delete[] message;
		//printf("parsedMessage.id == -1\n");
		if (parsedMessage.id == -1) {
			if (parsedMessage.nick.size() != 0) {
				for (int i = 0; i < SF->getSAMP()->getPlayers()->largestPlayerId; i++) {
					//printf("IsPlayerDefined\n");
					if (!SF->getSAMP()->getPlayers()->IsPlayerDefined(i)) continue;
					std::string playerName = SF->getSAMP()->getPlayers()->remotePlayerInfo[i]->nickname;
					//printf("playerName == parsedMessage.nick\n");
					if (playerName == parsedMessage.nick) {
						parsedMessage.id = i;
						break;
					}
				}
			}
		}
		else if (parsedMessage.nick.size() == 0) {
			//printf("parsedMessage.nick.size() == 0\n");
			if (SF->getSAMP()->getPlayers()->IsPlayerDefined(parsedMessage.id)) {
				parsedMessage.nick = SF->getSAMP()->getPlayers()->remotePlayerInfo[parsedMessage.id]->nickname;
			}
		}
		//printf("parsedMessage.id != -1 && parsedMessage.nick.size() != 0 && !bGenerating && messageOrder.size() == 0\n");
		if (parsedMessage.id != -1 && parsedMessage.nick.size() != 0 && !bGenerating && messageOrder.size() == 0 && parsedMessage.text.size() != 0) {
			if (bTargetAnswer && iTargetPlayerId != parsedMessage.id) return true;
			if (parsedMessage.id == SF->getSAMP()->getPlayers()->localPlayerInfo.id) return true;
			mutex.lock();
			messageOrder.push_back({EncodingUtil::ConvertCP1251ToUTF8(parsedMessage.text), false});
			mutex.unlock();
		}
	}

	return true;
}

void CALLBACK mainloop()
{
	static bool init = false;
	if (!init)
	{
		if (GAME == nullptr)
			return;
		if (GAME->GetSystemState() != eSystemState::GS_PLAYING_GAME)
			return;
		if (!SF->getSAMP()->IsInitialized())
			return;
		
		LoadSettings();

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui_ImplWin32_Init(GetActiveWindow());
		ImGui_ImplDX9_Init(SF->getRender()->getD3DDevice());
		ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5, 0.5);

		static const ImWchar ranges[] =
		{
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0400, 0x044F, // Cyrillic
			0,
		};

		ImFontConfig font_config;
		font_config.OversampleH = 1; //or 2 is the same
		font_config.OversampleV = 1;
		font_config.PixelSnapH = 1;

		io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 14, &font_config, ranges);

		SF->getRender()->registerD3DCallback(eDirect3DDeviceMethods::D3DMETHOD_PRESENT, Present);
		SF->getRender()->registerD3DCallback(eDirect3DDeviceMethods::D3DMETHOD_RESET, Reset);
		SF->getGame()->registerWndProcCallback(SFGame::MEDIUM_CB_PRIORITY, WndProcHandler);

		SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_INCOMING_RPC, OnIncomingRPC);

		SF->getSAMP()->registerChatCommand("gemini", OnGeminiCommand);
		std::thread(GeminiWorker).detach();

		init = true;
	}
	else {
		if (bGenerating)
			AddMessageJumpQ("Generating...", 100, 0, false);
		if (bWasMenu != bMenu) {
			bWasMenu = bMenu;
			SaveSettings();
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	switch (dwReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			SF->initPlugin(mainloop, hModule);
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
