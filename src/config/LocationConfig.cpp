#include "LocationConfig.hpp"
#include <iostream>
#include <algorithm>

LocationConfig::LocationConfig() :
	autoindex_(false)
{
}

LocationConfig::LocationConfig(const LocationConfig& other) :
	path_(other.path_),
	root_(other.root_),
	index_(other.index_),
	methods_(other.methods_),
	autoindex_(other.autoindex_),
	upload_store_(other.upload_store_),
	redirect_(other.redirect_)
{
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
	if (this != &other)
	{
		path_ = other.path_;
		root_ = other.root_;
		index_ = other.index_;
		methods_ = other.methods_;
		autoindex_ = other.autoindex_;
		upload_store_ = other.upload_store_;
		redirect_ = other.redirect_;
	}
	return *this;
}

LocationConfig::~LocationConfig()
{
}

void LocationConfig::setPath(const std::string& path) { path_ = path; }
void LocationConfig::setRoot(const std::string& root) { root_ = root; }

void LocationConfig::addIndex(const std::string& index)
{
	index_.push_back(index);
}

void LocationConfig::addMethod(const std::string& method)
{
	methods_.push_back(method);
}

void LocationConfig::setAutoIndex(bool autoindex) { autoindex_ = autoindex; }
void LocationConfig::setUploadStore(const std::string& store) { upload_store_ = store; }
void LocationConfig::setRedirect(const std::string& redirect) { redirect_ = redirect; }

const std::string& LocationConfig::getPath() const { return path_; }
const std::string& LocationConfig::getRoot() const { return root_; }
const std::vector<std::string>& LocationConfig::getIndexes() const { return index_; }
const std::vector<std::string>& LocationConfig::getMethods() const { return methods_; }
bool LocationConfig::getAutoIndex() const { return autoindex_; }
const std::string& LocationConfig::getUploadStore() const { return upload_store_; }
const std::string& LocationConfig::getRedirect() const { return redirect_; }

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
	if (methods_.empty())
		return false; // Default safe policy? Or allow all? Usually allow all if empty?
		// Nginx default is GET only if no limit_except.
		// For now, let's assume if empty, we might need default, but returning false is safer.
	for (size_t i = 0; i < methods_.size(); ++i)
	{
		if (methods_[i] == method)
			return true;
	}
	return false;
}

void LocationConfig::print() const
{
	std::cout << *this << std::endl;
}

std::ostream& operator<<(std::ostream& os, const LocationConfig& config)
{
	os << "  Location: " << config.path_ << "\n"
	   << "    Root: " << config.root_ << "\n"
	   << "    AutoIndex: " << (config.autoindex_ ? "on" : "off") << "\n"
	   << "    Upload Store: " << config.upload_store_ << "\n"
	   << "    Redirect: " << config.redirect_ << "\n";
	
	os << "    Indexes: ";
	for (size_t i = 0; i < config.index_.size(); ++i)
		os << config.index_[i] << " ";
	os << "\n";

	os << "    Methods: ";
	for (size_t i = 0; i < config.methods_.size(); ++i)
		os << config.methods_[i] << " ";
	os << "\n";

	return os;
}