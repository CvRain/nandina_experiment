// Umbrella module — consumers only need `import Nandina.Core`.
// Each sub-module has a single responsibility:
//   Nandina.Core.Color     — Color RGBA value type
//   Nandina.Core.Event     — EventType / MouseButton / Event
//   Nandina.Core.Signal    — Connection / EventSignal<>
//   Nandina.Core.Widget    — Widget base (tree / geometry / dirty / events)
//   Nandina.Core.Component — Component + RectangleComponent + FocusComponent
//   Nandina.Core.Button    — Button (composed of Component layers)
export module Nandina.Core;

export import Nandina.Core.Color;
export import Nandina.Core.Event;
export import Nandina.Core.Signal;
export import Nandina.Core.Widget;
export import Nandina.Core.Component;
export import Nandina.Core.Button;
export import Nandina.Reactive;

