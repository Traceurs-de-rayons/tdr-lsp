#include "LSPServer.hpp"
#include "core-utils.hpp"
#include <sstream>

void LSPServer::run()
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

std::string LSPServer::get_document_text(const std::string& uri)
{
	auto it = open_documents.find(uri);
	if (it != open_documents.end())
		return it->second;
	return "";
}

void LSPServer::handle_message(const std::string& content)
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
		else if (method == "textDocument/documentColor")
			handle_document_color(msg);
		else if (method == "textDocument/colorPresentation")
 		   handle_color_presentation(msg);
		else if (method == "exit")
			std::exit(0);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error handling message: " << e.what() << std::endl;
	}
}

void LSPServer::handle_initialize(const json& msg)
{
	json capabilities = {
		{"textDocumentSync", 1},
		{"completionProvider", {{"triggerCharacters", {"<", " "}}}},
		{"hoverProvider", true},
		{"colorProvider", true}
	};
	
	send_response(msg["id"], {{"capabilities", capabilities}});
}

void LSPServer::handle_did_open(const json& msg)
{
	std::string uri = msg["params"]["textDocument"]["uri"];
	std::string text = msg["params"]["textDocument"]["text"];
	open_documents[uri] = text;
	publish_diagnostics(uri, text);
}

void LSPServer::handle_did_change(const json& msg)
{
	std::string uri = msg["params"]["textDocument"]["uri"];
	std::string text = msg["params"]["contentChanges"][0]["text"];
	open_documents[uri] = text;
	publish_diagnostics(uri, text);
}

void LSPServer::handle_hover(const json& msg)
{
	int line = msg["params"]["position"]["line"];
	int character = msg["params"]["position"]["character"];
	std::string uri = msg["params"]["textDocument"]["uri"];

	std::string text = get_document_text(uri);
	auto result = service_.parse_content(text);

	std::string hover_text = SceneLanguageService::get_hover(result.ast, sch, line + 1, character + 1);

	if (hover_text.empty())
	{
		send_response(msg["id"], nullptr);
		return;
	}

	json hover_result = {
		{"contents", {
			{"kind", "markdown"},
			{"value", hover_text}
		}}
	};
	
	send_response(msg["id"], hover_result);
}

void LSPServer::publish_diagnostics(const std::string& uri, const std::string& text)
{
	auto result = SceneLanguageService::parse_content(text);
	
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

void LSPServer::handle_document_color(const json& msg)
{
	try
	{
		std::string text = get_document_text(msg["params"]["textDocument"]["uri"]);
		
		auto result = service_.parse_content(text);
		
		json colors = json::array();

		find_colors_in_ast(result.ast, colors);
		
		send_response(msg["id"], colors);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error in handle_document_color: " << e.what() << std::endl;
		send_response(msg["id"], json::array());
	}
	catch (...)
	{
		std::cerr << "Unknown error in handle_document_color" << std::endl;
		send_response(msg["id"], json::array());
	}
}

void LSPServer::handle_color_presentation(const json& msg)
{
	json color = msg["params"]["color"];
	double r = color["red"];
	double g = color["green"];
	double b = color["blue"];

	json presentations = json::array();

	char hex[8];
	sprintf(hex, "#%02X%02X%02X", 
			(int)(r * 255), 
			(int)(g * 255), 
			(int)(b * 255));
	presentations.push_back({{"label", hex}});
	
	char rgb[32];
	sprintf(rgb, "%d,%d,%d", 
			(int)(r * 255), 
			(int)(g * 255), 
			(int)(b * 255));
	presentations.push_back({{"label", rgb}});

	send_response(msg["id"], presentations);
}

void LSPServer::send_response(const json& id, const json& result)
{
	json response = {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"result", result}
	};
	send_json(response);
}

void LSPServer::send_notification(const std::string& method, const json& params)
{
	json notification = {
		{"jsonrpc", "2.0"},
		{"method", method},
		{"params", params}
	};
	send_json(notification);
}

void LSPServer::send_json(const json& msg)
{
	std::string content = msg.dump();
	std::cout << "Content-Length: " << content.size() << "\r\n\r\n" << content << std::flush;
}

std::optional<ColorInfo> parse_color(const std::string& value)
{
	if (value.size() == 7 && value[0] == '#')
	{
		int r, g, b;
		if (sscanf(value.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
			return ColorInfo{r / 255.0, g / 255.0, b / 255.0};
	}

	auto parts = cu::string::split(value, ',');
	if (parts.size() == 3)
	{
		try
		{
			int r = std::stoi(parts[0]);
			int g = std::stoi(parts[1]);
			int b = std::stoi(parts[2]);
			
			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
				return ColorInfo{r / 255.0, g / 255.0, b / 255.0};
		} catch (...) {}
	}
	
	return std::nullopt;
}

json LSPServer::node_text_range(const Node& node)
{
	auto pos = node.getTextBeginPos();
	return {
		{"start", {
			{"line", pos.first - 1},
			{"character", pos.second - 1}
		}},
		{"end", {
			{"line", pos.first - 1},
			{"character", pos.second + node.getText().size() - 1}
		}}
	};
}

void LSPServer::find_colors_in_ast(const Node& node, json& colors)
{
	auto tagsch = sch.getTagSchema(node.getIdentifier());

	if (tagsch && tagsch->text_type.has_value() && tagsch->text_type.value() == ValueType::COLOR && !node.getText().empty())
	{
		auto color_info = parse_color(node.getText());
		if (color_info)
		{
			colors.push_back({
				{"range", node_text_range(node)},
				{"color", {
					{"red", color_info->r},
					{"green", color_info->g},
					{"blue", color_info->b},
					{"alpha", 1.0}
				}}
			});
		}
	}
	
	for (auto it = node.getAttributes().begin(); it != node.getAttributes().end(); it++)
	{
		auto attrsch = sch.getAttributeSchema(node.getIdentifier(), it->first);
		if (attrsch && attrsch->type == ValueType::COLOR)
		{
			auto color_info = parse_color(it->second.content);
			if (color_info)
			{
				colors.push_back({
					{"range", {
						{"start", {
							{"line", it->second.content_line - 1},
							{"character", it->second.content_column - 1}
						}},
						{"end", {
							{"line", it->second.content_line - 1},
							{"character", it->second.content_column + it->second.content.size() - 1}
						}}
					}},
					{"color", {
						{"red", color_info->r},
						{"green", color_info->g},
						{"blue", color_info->b},
						{"alpha", 1.0}
					}}
				});
			}
		}
	}
	
	for (const auto& child : node.getChildren())
		find_colors_in_ast(child, colors);
}