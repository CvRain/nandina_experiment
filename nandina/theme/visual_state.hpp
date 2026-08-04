/**
 * theme/visual_state — 组件交互状态枚举。
 *
 * 从各组件样式文件中抽出，与具体样式解析解耦；配方规则与控件共享同一组枚举。
 */

#ifndef NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP
#define NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP

namespace nandina::theme
{
    /** Checkbox 交互状态。 */
    enum class CheckboxVisualState {
        normal,
        hovered,
        pressed,
        focused,
        disabled,
    };

    /** Slider 交互状态。 */
    enum class SliderVisualState {
        normal,
        hovered,
        dragging,
        focused,
        disabled,
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_VISUAL_STATE_HPP
