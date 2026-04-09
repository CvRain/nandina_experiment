// Umbrella module — consumers only need `import Nandina.Core`.
//
// Module layout
// ─────────────────────────────────────────────────────────────────────────────
// Nandina.Reactive          — all reactive primitives (single flat module)
//                             Connection / ScopedConnection / EventSignal<>
//                             State<T> / ReadState / Computed / Effect / EffectScope
//                             Prop<T> / StateList<T>
//
// Nandina.Core.Color        — Color RGBA value type
// Nandina.Core.Event        — EventType / MouseButton / Event
// Nandina.Core.Widget       — Widget base (tree / geometry / dirty / events)
// Nandina.Core.Component    — Component + RectangleComponent + FocusComponent
// Nandina.Core.CompositeComponent — CompositeComponent (build() lifecycle)
// Nandina.Core.Router       — Page + Router (push/pop/replace) + RouterView
// Nandina.Core.Button       — Button (composed of Component layers)
export module Nandina.Core;

// Reactive first: Widget sub-modules use Reactive types (e.g. Connection,
// EventSignal, State, EffectScope) via `import Nandina.Reactive`, and this
// single export makes them available to all consumers of Nandina.Core.
export import Nandina.Reactive;

export import Nandina.Core.Color;
export import Nandina.Core.Event;
export import Nandina.Core.Widget;
export import Nandina.Core.FreeWidget;
export import Nandina.Core.RenderLayer;
export import Nandina.Core.Component;
export import Nandina.Core.CompositeComponent;
export import Nandina.Core.Router;
export import Nandina.Core.Button;
