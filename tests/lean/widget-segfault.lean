open widget
open lean
open lean.parser
open interactive
open tactic

meta def component.stateless' {π α} (view : π → list (html α)) : component π α :=
component.mk
  α
  unit
  (λ _ _, ())
  (λ _ _ a, (⟨⟩, some a))
  (λ p _, view p )
  (λ _ _, ff)

meta def comp : component tactic_state string :=
component.stateless' $ λ ts,
  [h "h1" [] ["WIDGET"],
    h "img" [attr.val "src" "https://html.com/wp-content/uploads/flamingo.jpg"] []
  ]

@[user_command]
meta def widget_test (_ : parse (tk "#widgety")) : lean.parser unit :=
do  ⟨l,c⟩ ← cur_pos,
    tactic.save_widget ⟨l,c - ("#widgety").length - 1⟩ _, -- underscore here
    pure ()

run_cmd skip

#widgety
