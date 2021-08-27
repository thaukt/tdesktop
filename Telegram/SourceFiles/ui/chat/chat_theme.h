/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/effects/animations.h"
#include "base/timer.h"
#include "base/weak_ptr.h"

namespace Ui {

struct BubblePattern;

struct ChatPaintContext {
	not_null<const style::palette*> st;
	const BubblePattern *bubblesPattern = nullptr;
	QRect viewport;
	QRect clip;
	TextSelection selection;
	crl::time now = 0;

	void translate(int x, int y) {
		viewport.translate(x, y);
		clip.translate(x, y);
	}
	void translate(QPoint point) {
		translate(point.x(), point.y());
	}
	[[nodiscard]] ChatPaintContext translated(int x, int y) const {
		auto result = *this;
		result.translate(x, y);
		return result;
	}
	[[nodiscard]] ChatPaintContext translated(QPoint point) const {
		return translated(point.x(), point.y());
	}
};

struct ChatThemeBackground {
	QImage prepared;
	QImage preparedForTiled;
	QImage gradientForFill;
	std::optional<QColor> colorForFill;
	std::vector<QColor> colors;
	float64 patternOpacity = 1.;
	int gradientRotation = 0;
	bool isPattern = false;
	bool tile = false;
};

bool operator==(const ChatThemeBackground &a, const ChatThemeBackground &b);
bool operator!=(const ChatThemeBackground &a, const ChatThemeBackground &b);

struct CacheBackgroundRequest {
	ChatThemeBackground background;
	QSize area;
	int gradientRotationAdd = 0;
	float64 gradientProgress = 1.;

	explicit operator bool() const {
		return !background.prepared.isNull()
			|| !background.gradientForFill.isNull();
	}
};

bool operator==(
	const CacheBackgroundRequest &a,
	const CacheBackgroundRequest &b);
bool operator!=(
	const CacheBackgroundRequest &a,
	const CacheBackgroundRequest &b);

struct CacheBackgroundResult {
	QImage image;
	QImage gradient;
	QSize area;
	int x = 0;
	int y = 0;
};

struct CachedBackground {
	CachedBackground() = default;
	CachedBackground(CacheBackgroundResult &&result);

	QPixmap pixmap;
	QSize area;
	int x = 0;
	int y = 0;
};

struct BackgroundState {
	CachedBackground was;
	CachedBackground now;
	float64 shown = 1.;
};

struct ChatThemeDescriptor {
	uint64 id = 0;
	Fn<void(style::palette&)> preparePalette;
	Fn<ChatThemeBackground()> prepareBackground;
};

class ChatTheme final : public base::has_weak_ptr {
public:
	ChatTheme();

	// Expected to be invoked on a background thread. Invokes callbacks there.
	ChatTheme(ChatThemeDescriptor &&descriptor);

	[[nodiscard]] uint64 key() const;
	[[nodiscard]] not_null<const style::palette*> palette() const {
		return _palette.get();
	}

	void setBackground(ChatThemeBackground &&background);
	void updateBackgroundImageFrom(ChatThemeBackground &&background);
	const ChatThemeBackground &background() const {
		return _mutableBackground;
	}

	void setBubblesBackground(QImage image);
	const BubblePattern *bubblesBackgroundPattern() const {
		return _bubblesBackgroundPattern.get();
	}

	[[nodiscard]] ChatPaintContext preparePaintContext(
		QRect viewport,
		QRect clip);
	[[nodiscard]] const BackgroundState &backgroundState(QSize area);
	[[nodiscard]] rpl::producer<> repaintBackgroundRequests() const;
	void rotateComplexGradientBackground();

private:
	void cacheBackground();
	void cacheBackgroundNow();
	void cacheBackgroundAsync(
		const CacheBackgroundRequest &request,
		Fn<void(CacheBackgroundResult&&)> done = nullptr);
	void setCachedBackground(CacheBackgroundResult &&cached);
	[[nodiscard]] CacheBackgroundRequest currentCacheRequest(
		QSize area,
		int addRotation = 0) const;
	[[nodiscard]] bool readyForBackgroundRotation() const;
	void generateNextBackgroundRotation();

	uint64 _id = 0;
	std::unique_ptr<style::palette> _palette;
	ChatThemeBackground _mutableBackground;
	BackgroundState _backgroundState;
	Animations::Simple _backgroundFade;
	CacheBackgroundRequest _backgroundCachingRequest;
	CacheBackgroundResult _backgroundNext;
	int _backgroundAddRotation = 0;
	QSize _willCacheForArea;
	crl::time _lastAreaChangeTime = 0;
	std::optional<base::Timer> _cacheBackgroundTimer;
	CachedBackground _bubblesBackground;
	QImage _bubblesBackgroundPrepared;
	std::unique_ptr<BubblePattern> _bubblesBackgroundPattern;

	rpl::event_stream<> _repaintBackgroundRequests;

	rpl::lifetime _lifetime;

};

struct ChatBackgroundRects {
	QRect from;
	QRect to;
};
[[nodiscard]] ChatBackgroundRects ComputeChatBackgroundRects(
	QSize fillSize,
	QSize imageSize);

[[nodiscard]] QColor CountAverageColor(const QImage &image);
[[nodiscard]] QColor ThemeAdjustedColor(QColor original, QColor background);
[[nodiscard]] QImage PreprocessBackgroundImage(QImage image);
[[nodiscard]] std::optional<QColor> CalculateImageMonoColor(
	const QImage &image);
[[nodiscard]] QImage PrepareImageForTiled(const QImage &prepared);

[[nodiscard]] QImage ReadBackgroundImage(
	const QString &path,
	const QByteArray &content,
	bool gzipSvg);
[[nodiscard]] QImage GenerateBackgroundImage(
	QSize size,
	const std::vector<QColor> &bg,
	int gradientRotation,
	float64 patternOpacity = 1.,
	Fn<void(QPainter&)> drawPattern = nullptr);
[[nodiscard]] QImage PreparePatternImage(
	QImage pattern,
	const std::vector<QColor> &bg,
	int gradientRotation,
	float64 patternOpacity);
[[nodiscard]] QImage PrepareBlurredBackground(QImage image);
[[nodiscard]] QImage GenerateDitheredGradient(
	const std::vector<QColor> &colors,
	int rotation);

[[nodiscard]] ChatThemeBackground PrepareBackgroundImage(
	const QString &path,
	const QByteArray &bytes,
	bool gzipSvg,
	const std::vector<QColor> &colors,
	bool isPattern,
	float64 patternOpacity,
	bool isBlurred);

} // namespace Ui
