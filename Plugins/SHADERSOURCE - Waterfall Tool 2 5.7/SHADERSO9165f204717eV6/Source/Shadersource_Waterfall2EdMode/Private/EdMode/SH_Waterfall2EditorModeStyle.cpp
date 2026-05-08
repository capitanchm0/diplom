// Copyright 2021-2025 SHADERSOURCE.io / Pending Studios, All Rights Reserved.


#include "EdMode/SH_Waterfall2EditorModeStyle.h"
#include "Styling/SlateIconFinder.h"

//Style
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
//-----

TSharedPtr<FSlateStyleSet> FSH_Waterfall2EditorModeStyle::StyleInstance = NULL;

void FSH_Waterfall2EditorModeStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSH_Waterfall2EditorModeStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSH_Waterfall2EditorModeStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SH_Waterfall2EditorModeStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon40x40(40.f, 40.f);
const FVector2D Icon64x64(64.f, 64.f);
const FVector2D Icon16x16(16.0f, 16.0f);

TSharedRef<FSlateStyleSet> FSH_Waterfall2EditorModeStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SHADERSOURCE_WaterfallTool2")->GetBaseDir() / TEXT("Resources"));

	Style->Set("SHADERSOURCE.SHADERSOURCEIcon", new IMAGE_BRUSH(TEXT("SHADERSOURCE_Icon128"), Icon40x40));

	Style->Set("SHADERSOURCE.WaterfallTool2_Single", new IMAGE_BRUSH(TEXT("WFT2_Icon"), Icon40x40));

	Style->Set("SHADERSOURCE.WaterfallTool2", new IMAGE_BRUSH(TEXT("Icon128"), Icon40x40));

	Style->Set("SHADERSOURCE.Title.Banner", new IMAGE_BRUSH(TEXT("WaterfallTool2_Banner"), FVector2D(723, 126)));
	Style->Set("SHADERSOURCE.Title.Background", new IMAGE_BRUSH(TEXT("WaterfallTool2_Banner_Background"), FVector2D(603, 44)));
	Style->Set("SHADERSOURCE.Watermark.Background", new IMAGE_BRUSH(TEXT("ShaderSource_Logo_Background"), FVector2D(175, 201)));

	Style->Set("SHADERSOURCE.DropDown", new BOX_BRUSH(TEXT("DropDown"), 40.0f / 80.0f));

	Style->Set("ClassThumbnail.SH_Waterfall2", new IMAGE_BRUSH(TEXT("Icon128"), Icon64x64));
	Style->Set("ClassIcon.SH_Waterfall2", new IMAGE_BRUSH(TEXT("WFT2_Icon"), Icon16x16));

	Style->Set("SHADERSOURCE.VersionText", FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Regular", 8)));

	Style->Set("SHADERSOURCE.AdvancedButtonLabel", FTextBlockStyle()
		.SetFont(DEFAULT_FONT("Bold", 7))
		.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FSH_Waterfall2EditorModeStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSH_Waterfall2EditorModeStyle::Get()
{
	return *StyleInstance;
}