// This is the source code of AyuGram for Desktop.
//
// Local ("visual only") star gifts. See local_gifts.h.
#include "ayu/features/local_gifts/local_gifts.h"

#include "base/unixtime.h"
#include "chat_helpers/compose/compose_show.h"
#include "data/data_media_types.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_helpers.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"

namespace AyuFeatures::LocalGifts {
namespace {

struct Entry {
	PeerId to = 0;
	Data::SavedStarGift saved;
	FullMsgId itemId;
};

// Everything lives in memory only, keyed by the account's user id.
base::flat_map<UserId, std::vector<Entry>> &Storage() {
	static auto result = base::flat_map<UserId, std::vector<Entry>>();
	return result;
}

base::flat_set<FullMsgId> &Items() {
	static auto result = base::flat_set<FullMsgId>();
	return result;
}

base::flat_set<uint64> &SavedIds() {
	static auto result = base::flat_set<uint64>();
	return result;
}

} // namespace

QString Marker() {
	return QString::fromUtf8("\xF0\x9F\x91\x81 тест");
}

void Give(
		std::shared_ptr<ChatHelpers::Show> show,
		not_null<PeerData*> peer,
		const Data::StarGift &gift,
		const TextWithEntities &text,
		bool anonymous) {
	const auto session = &peer->session();
	const auto owner = &peer->owner();
	const auto history = owner->history(peer);
	const auto self = session->user();
	const auto date = base::unixtime::now();
	const auto id = owner->nextLocalMessageId();
	const auto unique = gift.unique;

	// 1. Service message in the chat, marked as local.
	auto serviceText = unique
		? tr::lng_action_gift_unique_sent(tr::now, tr::marked)
		: tr::lng_action_gift_sent(
			tr::now,
			lt_cost,
			TextWithEntities{
				tr::lng_action_gift_for_stars(
					tr::now,
					lt_count,
					gift.stars),
			},
			tr::marked);
	serviceText.text += u" "_q + Marker();

	const auto item = history->makeMessage({
		.id = id,
		.flags = (MessageFlag::Local | MessageFlag::Outgoing),
		.from = self->id,
		.date = date,
	}, PreparedServiceText{ std::move(serviceText) });

	item->setLocalStarGiftMedia(Data::GiftCode{
		.stargiftId = unique ? unique->initialGiftId : gift.id,
		.document = gift.document.get(),
		.stargiftReleasedBy = gift.releasedBy,
		.unique = unique,
		.message = text,
		.messageAuthor = anonymous ? nullptr : self.get(),
		.giftTitle = gift.resellTitle,
		.limitedCount = gift.limitedCount,
		.limitedLeft = gift.limitedLeft,
		.count = unique ? 0 : gift.stars,
		.type = Data::GiftType::StarGift,
		.messageFromUniqueAction = (unique != nullptr),
		.anonymous = anonymous,
		.saved = true,
	});
	Items().emplace(item->fullId());
	history->addNewLocalMessage(item);

	// 2. Entry for the recipient's profile gift list.
	const auto savedId = uint64(id.bare);
	SavedIds().emplace(savedId);
	auto &list = Storage()[session->userId()];
	list.insert(begin(list), Entry{
		.to = peer->id,
		.saved = Data::SavedStarGift{
			.info = gift,
			.manageId = (peer->isUser()
				? Data::SavedStarGiftId::User(id)
				: Data::SavedStarGiftId::Chat(peer, savedId)),
			.message = text,
			.fromId = anonymous ? PeerId() : self->id,
			.date = date,
			.anonymous = anonymous,
			.mine = peer->isSelf(),
		},
		.itemId = item->fullId(),
	});

	if (const auto window = show->resolveWindow()) {
		window->showPeerHistory(peer);
	}
	show->showToast(u"Подарок показан только у вас "_q + Marker());
}

void GiveUnique(
		std::shared_ptr<ChatHelpers::Show> show,
		not_null<PeerData*> peer,
		std::shared_ptr<Data::UniqueGift> unique,
		const TextWithEntities &text,
		bool anonymous) {
	Expects(unique != nullptr);

	Give(show, peer, Data::StarGift{
		.id = unique->initialGiftId,
		.unique = unique,
		.document = unique->model.document,
		.resellTitle = unique->title,
	}, text, anonymous);
}

std::vector<Data::SavedStarGift> ForPeer(not_null<PeerData*> peer) {
	auto result = std::vector<Data::SavedStarGift>();
	const auto &storage = Storage();
	const auto i = storage.find(peer->session().userId());
	if (i == end(storage)) {
		return result;
	}
	for (const auto &entry : i->second) {
		if (entry.to == peer->id) {
			result.push_back(entry.saved);
		}
	}
	return result;
}

bool IsLocal(not_null<HistoryItem*> item) {
	return item->isLocal() && Items().contains(item->fullId());
}

bool IsLocal(const Data::SavedStarGiftId &id) {
	if (!id) {
		return false;
	}
	const auto value = id.isUser()
		? uint64(id.userMessageId().bare)
		: id.chatSavedId();
	return SavedIds().contains(value);
}

} // namespace AyuFeatures::LocalGifts
