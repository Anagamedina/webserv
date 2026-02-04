#include "LocationConfig.hpp"
#include "ConfigException.hpp"
#include <iostream>


LocationConfig::LocationConfig() :
	autoindex_(false), redirect_code_(0)
{
}

LocationConfig::LocationConfig(const LocationConfig& other) :
	path_(other.path_),
	root_(other.root_),
	index_(other.index_),
	allowed_methods_(other.allowed_methods_),
	autoindex_(other.autoindex_),
	upload_store_(other.upload_store_),
	redirect_code_(other.redirect_code_),
	redirect_url_(other.redirect_url_)
{
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
	if (this != &other)
	{
		path_ = other.path_;
		root_ = other.root_;
		index_ = other.index_;
		allowed_methods_ = other.allowed_methods_;
		autoindex_ = other.autoindex_;
		upload_store_ = other.upload_store_;
		redirect_code_ = other.redirect_code_;
		redirect_url_ = other.redirect_url_;
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
	allowed_methods_.push_back(method);
}

void LocationConfig::setAutoIndex(const bool autoindex) { autoindex_ = autoindex; }

void LocationConfig::setUploadStore(const std::string& store)
{
	upload_store_ = store;
}

void LocationConfig::setRedirectCode(const int integerCode)
{
	/*
	if (integerCode < 300 || integerCode > 399)
	{
		throw ConfigException(config::errors::invalid_redirect_code);
	}
	*/
	redirect_code_ = integerCode;
}

void LocationConfig::setRedirectUrl(const std::string& redirectUrl)
{
	redirect_url_ = redirectUrl;
}

const std::string& LocationConfig::getPath() const { return path_; }
const std::string& LocationConfig::getRoot() const { return root_; }

const std::vector<std::string>& LocationConfig::getIndexes() const
{
	return index_;
}

const std::vector<std::string>& LocationConfig::getMethods() const
{
	return allowed_methods_;
}

bool LocationConfig::getAutoIndex() const { return autoindex_; }

const std::string& LocationConfig::getUploadStore() const
{
	return upload_store_;
}

int LocationConfig::getRedirectCode() const { return redirect_code_; }

const std::string& LocationConfig::getRedirectUrl() const
{
	return redirect_url_;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
	if (!allowed_methods_.empty())
	{
		// Default safe policy? Or allow all? Usually allow all if empty?
		// Nginx default is GET only if no limit_except.
		// For now, let's assume if empty, we might need default, but returning false is safer.
		for (size_t i = 0; i < allowed_methods_.size(); ++i)
		{
			if (allowed_methods_[i] == method)
				return true;
		}
	}
	return false;
}

void LocationConfig::print() const
{
	std::cout << *this << std::endl;
}
