// This is the source code of AyuGram for Desktop.
//
// Local ("visual only") star gifts.
// Gifts created here exist ONLY in this client's memory:
// nothing is sent to Telegram servers, no stars are spent,
// nobody else can see them, and they disappear after restart.
// Every such gift is explicitly marked as local in the UI.
#pragma once

#include "data/data_star_gift.h"

class HistoryItem;
class PeerData;

namespace ChatHelpers {
class Show;
} // namespace ChatHelpers

namespace AyuFeatures::LocalGifts {

// Short marker shown on every local gift (chat message + profile badge).
[[nodiscard]] QString Marker();

// Creates a local gift service message in the chat with `peer`
// and adds the gift to `peer` profile gift list (locally).
// Works both for regular gifts and for unique (NFT) ones
// (gift.unique != nullptr).
void Give(
	std::shared_ptr<ChatHelpers::Show> show,
	not_null<PeerData*> peer,
	const Data::StarGift &gift,
	const TextWithEntities &text,
	bool anonymous);

// Same for a unique (NFT) gift, e.g. from the resale market.
void GiveUnique(
	std::shared_ptr<ChatHelpers::Show> show,
	not_null<PeerData*> peer,
	std::shared_ptr<Data::UniqueGift> unique,
	const TextWithEntities &text,
	bool anonymous);

// Local gifts "received" by `peer`, newest first.
[[nodiscard]] std::vector<Data::SavedStarGift> ForPeer(
	not_null<PeerData*> peer);

[[nodiscard]] bool IsLocal(not_null<HistoryItem*> item);
[[nodiscard]] bool IsLocal(const Data::SavedStarGiftId &id);

} // namespace AyuFeatures::LocalGifts
