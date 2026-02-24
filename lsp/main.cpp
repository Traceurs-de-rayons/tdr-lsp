#include <iostream>
#include <string>
#include <sstream>
#include "tdr/LanguageService.hpp"
#include "json.hpp"

using json = nlohmann::json;

class LSPServer
{
	
public:
	void run()
	{
		std::string line;

		while (std::getline(std::cin, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			
			if (line.find("Content-Length:") == 0)
			{
				int length = std::stoi(line.substr(16));
				
				while (std::getline(std::cin, line))
				{
					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					if (line.empty())
						break;
				}
				
				std::string content(length, ' ');
				std::cin.read(&content[0], length);
				
				handle_message(content);
			}
		}
	}
	
private:
	void handle_message(const std::string& content)
	{
		json msg;
		try
		{
			msg = json::parse(content);
		}
		catch (const std::exception& e)
		{
			std::cerr << "JSON parse error: " << e.what() << std::endl;
			return;
		}
		
		try
		{
			if (!msg.contains("method"))
				return;
			
			std::string method = msg["method"];
			
			if (method == "initialize")
				handle_initialize(msg);
			else if (method == "textDocument/didOpen")
				handle_did_open(msg);
			else if (method == "textDocument/didChange")
				handle_did_change(msg);
			else if (method == "textDocument/hover")
				handle_hover(msg);
			else if (method == "shutdown")
				send_response(msg["id"], nullptr);
			else if (method == "exit")
				std::exit(0);
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error handling message: " << e.what() << std::endl;
		}
	}
	
	void handle_initialize(const json& msg)
	{
		json capabilities = {
			{"textDocumentSync", 1},
			{"completionProvider", {{"triggerCharacters", {"<", " "}}}},
			{"hoverProvider", true}
		};
		
		send_response(msg["id"], {{"capabilities", capabilities}});
	}
	
	void handle_did_open(const json& msg)
	{
		std::string uri = msg["params"]["textDocument"]["uri"];
		std::string text = msg["params"]["textDocument"]["text"];
		publish_diagnostics(uri, text);
	}
	
	void handle_did_change(const json& msg)
	{
		std::string uri = msg["params"]["textDocument"]["uri"];
		std::string text = msg["params"]["contentChanges"][0]["text"];
		publish_diagnostics(uri, text);
	}
	
	void handle_hover(const json& msg)
	{
		int line = msg["params"]["position"]["line"];
		int character = msg["params"]["position"]["character"];
		std::string uri = msg["params"]["textDocument"]["uri"];
		
		json hover_result = {
			{"contents", "Hover information for position (" + std::to_string(line) + "," + std::to_string(character) + ")"}
		};
		
		send_response(msg["id"], hover_result);
	}
	
	void publish_diagnostics(const std::string& uri, const std::string& text)
	{
		auto result = sceneIO::tdr::SceneLanguageService::parse_content(text);
		
		json diagnostics = json::array();
		for (const auto& error : result.errors)
		{
			diagnostics.push_back({
				{"range", {
					{"start", {{"line", error.location.line - 1}, {"character", error.location.column - 1}}},
					{"end", {{"line", error.location.line - 1}, {"character", error.location.column}}}
				}},
				{"severity", error.getErrorLevel()},
				{"message", error.getMessage()}
			});
		}
		
		send_notification("textDocument/publishDiagnostics", {
			{"uri", uri},
			{"diagnostics", diagnostics}
		});
	}
	
	void send_response(const json& id, const json& result)
	{
		json response = {
			{"jsonrpc", "2.0"},
			{"id", id},
			{"result", result}
		};
		send_json(response);
	}
	
	void send_notification(const std::string& method, const json& params)
	{
		json notification = {
			{"jsonrpc", "2.0"},
			{"method", method},
			{"params", params}
		};
		send_json(notification);
	}
	
	void send_json(const json& msg) {
		std::string content = msg.dump();
		std::cout << "Content-Length: " << content.size() << "\r\n\r\n" << content << std::flush;
	}
};

int main()
{
	LSPServer server;
	server.run();
	return 0;
}