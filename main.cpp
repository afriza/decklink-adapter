#include <vector>
#include <string>
#include <iostream>

#include "platform.h"

const std::vector<std::tuple<BMDVideoInputConversionMode,std::string,std::string>> kVideoInputConversion = {
	std::make_tuple(bmdNoVideoInputConversion,                      "None",                "No video input conversion"),
	std::make_tuple(bmdVideoInputLetterboxDownconversionFromHD1080, "HD1080toSDLetterbox", "Letterbox from HD1080 to SD video input down conversion"),
	std::make_tuple(bmdVideoInputAnamorphicDownconversionFromHD1080,"HD1080toSDAnamorphic","Anamorphic from HD1080 to SD video input down conversion"),
	std::make_tuple(bmdVideoInputLetterboxDownconversionFromHD720,  "HD720toSDLetterbox",  "Letterbox from HD720 to SD video input down conversion"),
	std::make_tuple(bmdVideoInputAnamorphicDownconversionFromHD720, "HD720toSDAnamorphic", "Anamorphic from HD720 to SD video input down conversion"),
	std::make_tuple(bmdVideoInputLetterboxUpconversion,             "SDtoHDLetterbox",     "Letterbox video input up conversion"),
	std::make_tuple(bmdVideoInputAnamorphicUpconversion,            "SDtoHDAnamorphic",    "Anamorphic video input up conversion"),
};
enum {
	kVideoInputConversionValue = 0,
	kVideoInputConversionKey,
	kVideoInputConversionDescription,
};

int getVideoInputConversionIndexFromValue(BMDVideoInputConversionMode value) {
	for (size_t i = 0; i < kVideoInputConversion.size(); i++)
	{
		if (std::get<kVideoInputConversionValue>(kVideoInputConversion[i]) == value) {
			return i;
		}
	}
	return -1;
}

// Taken from https://stackoverflow.com/a/4654718/109747
bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(), 
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

int getVideoInputConversionIndexFromKey(std::string key) {
	for (size_t i = 0; i < kVideoInputConversion.size(); i++)
	{
		if (std::get<kVideoInputConversionKey>(kVideoInputConversion[i]) == key) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char* argv[]) {
	int deckLinkIndex = -1;
	int setVicmIndex  = -1;

	int idx;

	IDeckLinkIterator*			deckLinkIterator		= NULL;
	IDeckLink*					deckLink				= NULL;
	IDeckLink*					selectedDeckLink		= NULL;
	IDeckLinkAttributes*		deckLinkAttributes		= NULL;
	IDeckLinkConfiguration*		deckLinkConfiguration	= NULL;

	std::vector<std::string>	deckLinkDisplayNames;

	HRESULT						result;
	// Process the command line arguments
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-d") == 0) {
			if (i+1 == argc) {
				fprintf(stderr,"-d requires an integer argument\n");
				return 1;
			}
			deckLinkIndex = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-vicmIndex") == 0) { // video input conversion mode
			if (i+1 == argc || !is_number(argv[i+1])) {
				fprintf(stderr,"-vicmIndex requires an integer argument\n");
				return 1;
			}
			setVicmIndex = atoi(argv[++i]);
			if (setVicmIndex < 0 || setVicmIndex >= (int)kVideoInputConversion.size()) {
				fprintf(stderr,"-vicmIndex must be between 0 and %lu (inclusive)", kVideoInputConversion.size()-1);
				return 1;
			}
		}
		else if (strcmp(argv[i], "-vicmKey") == 0) {
			if (i+1 == argc) {
				std::cerr << "-vicmKey requires a string argument" << std::endl;
				return 1;
			}
			setVicmIndex = getVideoInputConversionIndexFromKey(argv[++i]);
			if (setVicmIndex == -1) {
				std::cerr << "no matching value for -vicmKey of "<< argv[i] << std::endl;
				return 1;
			}
		}
		else
		{
			// Unknown argument on command line
			fprintf(stderr, "Unknown argument %s\n", argv[i]);
		}
	}

	if (deckLinkIndex < 0)
	{
		fprintf(stderr, "You must select a device\n");
		return 1;
	}

	// Create an IDeckLinkIterator object to enumerate all DeckLink cards in the system
	result = GetDeckLinkIterator(&deckLinkIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.\n");
		goto bail;
	}

	// Obtain the required DeckLink device
	idx = 0;

	while ((result = deckLinkIterator->Next(&deckLink)) == S_OK)
	{
		dlstring_t deckLinkName;

		result = deckLink->GetDisplayName(&deckLinkName);
		if (result == S_OK)
		{
			deckLinkDisplayNames.push_back(DlToStdString(deckLinkName));
			DeleteString(deckLinkName);
		}

		if (idx++ == deckLinkIndex)
			selectedDeckLink = deckLink;
		else
			deckLink->Release();
	}

	if (selectedDeckLink != NULL)
	{
		if ((result=selectedDeckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration)) != S_OK)
		{
			fprintf(stderr, "Unable to query IDeckLinkConfiguration interface\n");
			goto bail;
		}

		if ((result=selectedDeckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes)) != S_OK)
		{
			fprintf(stderr, "Unable to query IDeckLinkAttributes interface\n");
			goto bail;
		}
	}

	if (deckLinkConfiguration != NULL) {
		int videoInputConversionIndex;
		int64_t videoInputConversion = -1;
		if ((result=deckLinkConfiguration->GetInt(bmdDeckLinkConfigVideoInputConversionMode, &videoInputConversion)) != S_OK) {
			fprintf(stderr, "Failed to get DeckLinkConfigVideoInputConversionMode. error %08x\n", result);
			goto bail;
		}
		
		videoInputConversionIndex = getVideoInputConversionIndexFromValue(videoInputConversion);
		if (videoInputConversion != -1) {
			fprintf(stderr,"current input video conversion: [%d] %s\n", videoInputConversionIndex, std::get<kVideoInputConversionDescription>(kVideoInputConversion[videoInputConversionIndex]).c_str());
		}

		if (0 <= setVicmIndex && setVicmIndex < (int)kVideoInputConversion.size()) {
			videoInputConversion = std::get<kVideoInputConversionValue>(kVideoInputConversion[setVicmIndex]);
			result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConversionMode, videoInputConversion);
			if (result == S_OK) {
				std::cerr << "Success to set Video Input Conversion Mode to [" <<setVicmIndex<< "] " << std::get<kVideoInputConversionDescription>(kVideoInputConversion[setVicmIndex]) << std::endl;
				result = deckLinkConfiguration->WriteConfigurationToPreferences();
				if (result == S_OK) {
					std::cerr << "Success saving config to preferences." << std::endl;
				} else {
					std::cerr << "Failed to save config to preferences." << std::endl;
				}
			} else {
				std::cerr << "Failed to set Video Input Conversion Mode to [" <<setVicmIndex<< "] " << std::get<kVideoInputConversionDescription>(kVideoInputConversion[setVicmIndex]) << std::endl;
			}
		}
	}

bail:
	if (deckLinkConfiguration != NULL)
	{
		deckLinkConfiguration->Release();
		deckLinkConfiguration = NULL;
	}

	if (deckLinkAttributes != NULL)
	{
		deckLinkAttributes->Release();
		deckLinkAttributes = NULL;
	}

	if (selectedDeckLink != NULL)
	{
		selectedDeckLink->Release();
		selectedDeckLink = NULL;
	}

	if (deckLinkIterator != NULL)
	{
		deckLinkIterator->Release();
		deckLinkIterator = NULL;
	}

	return(result == S_OK) ? 0 : 1;
}