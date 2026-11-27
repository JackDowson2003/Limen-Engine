#include "UI/EditorNativeMenu.h"

#include <utility>

#import <AppKit/AppKit.h>

#include "UI/Panels/SceneToolbarPanel.h"

@interface LimenEditorMenuTarget : NSObject <NSMenuItemValidation>
{
@public
    bool HasProject;
    std::function<void(LimenEditor::SceneToolbarAction)> SceneAction;
    std::function<void()> DeleteSelection;
}

- (void)performSceneCommand:(NSMenuItem *)item;
- (void)deleteSceneSelection:(NSMenuItem *)item;
@end

@implementation LimenEditorMenuTarget

- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    // 原生文件面板打开时，由面板处理自己的 Edit 和快捷键。
    if (!HasProject || NSApp.modalWindow)
        return NO;

    if (item.action == @selector(performSceneCommand:))
        return static_cast<bool>(SceneAction);
    return item.action == @selector(deleteSceneSelection:) && static_cast<bool>(DeleteSelection);
}

- (void)performSceneCommand:(NSMenuItem *)item
{
    NSEvent *event = NSApp.currentEvent;
    if ([self validateMenuItem:item] && !(event.type == NSEventTypeKeyDown && event.isARepeat))
        SceneAction(static_cast<LimenEditor::SceneToolbarAction>(item.tag));
}

- (void)deleteSceneSelection:(NSMenuItem *)item
{
    if ([self validateMenuItem:item])
        DeleteSelection();
}

@end

namespace LimenEditor
{
    namespace
    {
        // AppKit 的 NSMenuItem.target 不拥有目标，菜单模块持有完整的回调生命周期。
        LimenEditorMenuTarget *s_Target;
        NSMenuItem *s_FileMenuItem;
        NSMenuItem *s_EditMenuItem;
        NSMenu *s_OwnedMainMenu;

        void AddSceneCommand(NSMenu *menu, NSString *title, NSString *key,
                             const SceneToolbarAction action,
                             const NSEventModifierFlags modifiers = NSEventModifierFlagCommand)
        {
            NSMenuItem *item = [menu addItemWithTitle:title
                                              action:@selector(performSceneCommand:)
                                       keyEquivalent:key];
            item.target = s_Target;
            item.tag = static_cast<NSInteger>(action);
            item.keyEquivalentModifierMask = modifiers;
            item.enabled = s_Target->HasProject && static_cast<bool>(s_Target->SceneAction);
        }
    }

    void InstallEditorNativeMenu(
        const bool hasProject,
        std::function<void(SceneToolbarAction)> sceneAction,
        std::function<void()> deleteSelection)
    {
        @autoreleasepool
        {
            UninstallEditorNativeMenu();
            NSMenu *mainMenu = NSApp.mainMenu;
            if (!mainMenu)
            {
                // GLFW 正常启动会提供应用菜单。为未提供菜单的宿主保留 About 和 Quit。
                mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
                NSMenuItem *appItem = [mainMenu addItemWithTitle:@"" action:nullptr keyEquivalent:@""];
                NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Limen Editor"];
                appItem.submenu = appMenu;
                [appMenu addItemWithTitle:@"About Limen Editor"
                                   action:@selector(orderFrontStandardAboutPanel:)
                            keyEquivalent:@""];
                [appMenu addItem:[NSMenuItem separatorItem]];
                [appMenu addItemWithTitle:@"Quit Limen Editor" action:@selector(terminate:) keyEquivalent:@"q"];
                NSApp.mainMenu = mainMenu;
                s_OwnedMainMenu = mainMenu;
            }

            s_Target = [[LimenEditorMenuTarget alloc] init];
            s_Target->HasProject = hasProject;
            s_Target->SceneAction = std::move(sceneAction);
            s_Target->DeleteSelection = std::move(deleteSelection);

            NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
            AddSceneCommand(fileMenu, @"New Scene", @"n", SceneToolbarAction::New);
            AddSceneCommand(fileMenu, @"Open Scene…", @"o", SceneToolbarAction::Open);
            [fileMenu addItem:[NSMenuItem separatorItem]];
            AddSceneCommand(fileMenu, @"Save Scene", @"s", SceneToolbarAction::Save);
            // Shift 会改变 AppKit 匹配的字符，因此这里使用大写 S。
            AddSceneCommand(fileMenu, @"Save Scene As…", @"S", SceneToolbarAction::SaveAs,
                            NSEventModifierFlagCommand | NSEventModifierFlagShift);

            NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
            [editMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
            [editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
            [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
            [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
            [editMenu addItem:[NSMenuItem separatorItem]];
            NSMenuItem *deleteItem = [editMenu addItemWithTitle:@"Delete Selection"
                                                        action:@selector(deleteSceneSelection:)
                                                 keyEquivalent:@""];
            deleteItem.target = s_Target;
            deleteItem.enabled = hasProject && static_cast<bool>(s_Target->DeleteSelection);

            s_FileMenuItem = [[NSMenuItem alloc] initWithTitle:@"File" action:nullptr keyEquivalent:@""];
            s_FileMenuItem.submenu = fileMenu;
            s_EditMenuItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nullptr keyEquivalent:@""];
            s_EditMenuItem.submenu = editMenu;

            // 保留 GLFW 的应用与窗口菜单，插入编辑器自己的 File 和 Edit。
            const NSInteger windowIndex = NSApp.windowsMenu
                ? [mainMenu indexOfItemWithSubmenu:NSApp.windowsMenu] : -1;
            const NSInteger insertionIndex = windowIndex >= 0 ? windowIndex : mainMenu.numberOfItems;
            [mainMenu insertItem:s_FileMenuItem atIndex:insertionIndex];
            [mainMenu insertItem:s_EditMenuItem atIndex:insertionIndex + 1];
        }
    }

    void UninstallEditorNativeMenu()
    {
        [s_FileMenuItem.menu removeItem:s_FileMenuItem];
        [s_EditMenuItem.menu removeItem:s_EditMenuItem];
        if (s_OwnedMainMenu && NSApp.mainMenu == s_OwnedMainMenu)
            NSApp.mainMenu = nil;
        s_FileMenuItem = nil;
        s_EditMenuItem = nil;
        s_OwnedMainMenu = nil;
        s_Target = nil;
    }
}
