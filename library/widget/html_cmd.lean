open lean
open lean.parser
open interactive
open tactic
open widget

-- [note] stolen from mathlib
private meta def eval_pexpr (α) [reflected α] (e : pexpr) : tactic α :=
to_expr ``(%%e : %%(reflect α)) ff ff >>= eval_expr α

/-- Accepts terms with the type `component tactic_state string` or `html empty` and
renders them interactively. -/
@[user_command]
meta def show_widget_cmd (x : parse $ tk "#html") : parser unit := do
  ⟨l,c⟩ ← cur_pos,
  y ← parser.pexpr,
  comp ← parser.of_tactic ((do
    eval_pexpr (component tactic_state string) y
  ) <|> (do
    htm : html empty ← eval_pexpr (html empty) y,
    c : component unit empty ← pure $ component.stateless (λ _, [htm]),
    pure $ component.ignore_props $ component.ignore_action $ c
  )),
  save_widget ⟨l,c - ("#html").length - 1⟩ comp,
  trace "successfully rendered widget"
  pure ()

run_cmd skip
