/* Copyright (C) 2013 Liru Færs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */

#include "llviewerprecompiledheaders.h"

#include "lfsimfeaturehandler.h"

#include "llagent.h"
#include "lluictrlfactory.h" // For OnLook Special UI.
#include "llviewerregion.h"
#include "hippogridmanager.h"

LFSimFeatureHandler::LFSimFeatureHandler()
: mSupportsExport(false)
, mDestinationGuideURL(gSavedSettings.getString("DestinationGuideURL"))
, mSearchURL(gSavedSettings.getString("SearchURL"))
, mSayRange(20)
, mShoutRange(100)
, mWhisperRange(10)
{
	if (!gHippoGridManager->getCurrentGrid()->isSecondLife()) // Remove this line if we ever handle SecondLife sim features
		gAgent.addRegionChangedCallback(boost::bind(&LFSimFeatureHandler::handleRegionChange, this));
}

ExportPolicy LFSimFeatureHandler::exportPolicy() const
{
	return gHippoGridManager->getCurrentGrid()->isSecondLife() ? ep_creator_only : (mSupportsExport ? ep_export_bit : ep_full_perm);
}

void LFSimFeatureHandler::handleRegionChange()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		if (region->getFeaturesReceived())
		{
			setSupportedFeatures();
		}
		else
		{
			region->setFeaturesReceivedCallback(boost::bind(&LFSimFeatureHandler::setSupportedFeatures, this));
		}
	}
}

template<typename T>
void has_feature_or_default(SignaledType<T>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? static_cast<T>(features[feature]) : type.getDefault();
}
template<>
void has_feature_or_default(SignaledType<U32>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? features[feature].asInteger() : type.getDefault();
}

void close_if_built(const std::string& name) { if (LLFloater* floater = LLUICtrlFactory::instance().getBuiltFloater(name)) floater->close(); }
void LFSimFeatureHandler::setSupportedFeatures()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		LLSD info;
		region->getSimulatorFeatures(info);
		//bool hg(); // Singu Note: There should probably be a flag for this some day.
		U8 onlook_mask = 0;
		if (info.has("OpenSimExtras")) // OpenSim specific sim features
		{
			// For definition of OpenSimExtras please see
			// http://opensimulator.org/wiki/SimulatorFeatures_Extras
			const LLSD& extras(info["OpenSimExtras"]);
			has_feature_or_default(mSupportsExport, extras, "ExportSupported");
			//if (hg)
			{
				has_feature_or_default(mDestinationGuideURL, extras, "destination-guide-url");
				mMapServerURL = extras.has("map-server-url") ? extras["map-server-url"].asString() : "";
				has_feature_or_default(mSearchURL, extras, "search-server-url");
			}
			has_feature_or_default(mSayRange, extras, "say-range");
			has_feature_or_default(mShoutRange, extras, "shout-range");
			has_feature_or_default(mWhisperRange, extras, "whisper-range");
			if (extras.has("camera-only-mode")) onlook_mask |= 1;
			if (extras.has("special-ui"))
			{
				mSpecialUI = extras["special-ui"]["toolbar"].asString();
				onlook_mask |= 2;
				if (extras["special-ui"].has("floaters"))
				{
					const LLSD& floaters(extras["special-ui"]["floaters"]);
					for (LLSD::map_const_iterator it = mSpecialFloaters.beginMap(), end = mSpecialFloaters.endMap(); it != end; ++it)
						if (!floaters.has(it->first)) close_if_built(it->first);
					mSpecialFloaters = floaters;
				}
				else resetSpecialFloaters();
			}
			else mSpecialUI.reset();
		}
		else // OpenSim specifics are unsupported reset all to default
		{
			mSupportsExport.reset();
			//if (hg)
			{
				mDestinationGuideURL.reset();
				mMapServerURL = "";
				mSearchURL.reset();
			}
			mSayRange.reset();
			mShoutRange.reset();
			mWhisperRange.reset();
			mSpecialUI.reset();
			resetSpecialFloaters();
		}
		mOnLookMask = onlook_mask;
	}
}

void LFSimFeatureHandler::resetSpecialFloaters()
{
	for (LLSD::map_const_iterator it = mSpecialFloaters.beginMap(), end = mSpecialFloaters.endMap(); it != end; ++it)
		close_if_built(it->first);
	mSpecialFloaters.clear();
}

boost::signals2::connection LFSimFeatureHandler::setSupportsExportCallback(const SignaledType<bool>::slot_t& slot)
{
	return mSupportsExport.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setDestinationGuideURLCallback(const SignaledType<std::string>::slot_t& slot)
{
	return mDestinationGuideURL.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSearchURLCallback(const SignaledType<std::string>::slot_t& slot)
{
	return mSearchURL.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSayRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mSayRange.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setShoutRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mShoutRange.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setWhisperRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mWhisperRange.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setOnLookMaskCallback(const SignaledType<U8>::slot_t& slot)
{
	return mOnLookMask.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSpecialUICallback(const SignaledType<std::string>::slot_t& slot)
{
	return mSpecialUI.connect(slot);
}
