/*
Copyright (c) 2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: E.W.Ayers
*/
#pragma once
#include <map>
#include <algorithm>
#include <vector>
#include <string>
#include "util/list.h"
#include "util/pair.h"
#include "kernel/expr.h"
#include "library/vm/vm.h"
#include "library/io_state_stream.h"
#include "frontends/lean/json.h"

namespace lean {

class vdom;

enum class vdom_kind { String, Element, ComponentInstance};

class vdom_cell {
  MK_LEAN_RC();
  void dealloc() { delete this; }
protected:
    friend vdom;
public:
  vdom_cell() : m_rc(0) {}
  virtual ~vdom_cell() {}
  virtual json to_json(list<unsigned> const &) { return json(); };
  virtual optional<std::string> key() { return optional<std::string>(); };
  virtual void reconcile(vdom const &) { }
};

class vdom {
private:
  vdom_cell * m_ptr;
  friend class vdom_cell;
public:
  vdom(vdom_cell * ptr) : m_ptr(ptr) { lean_assert(m_ptr);  m_ptr->inc_ref(); }
  vdom(vdom const & s) : m_ptr(s.m_ptr) { if (m_ptr) m_ptr->inc_ref(); }
  vdom(vdom && s) : m_ptr(s.m_ptr) { s.m_ptr = nullptr; }
  ~vdom() {if (m_ptr) m_ptr->dec_ref(); }
  vdom & operator=(vdom const & s) { LEAN_COPY_REF(s); }
  vdom & operator=(vdom && s) { LEAN_MOVE_REF(s); }
  void reconcile(vdom const & old_vdom) { m_ptr->reconcile(old_vdom); }
  vdom_cell * raw() const { return m_ptr; }
  json to_json(list<unsigned> const & route = {}) const { return m_ptr->to_json(route); }
  optional<std::string> key() { return m_ptr->key(); }
};

struct vdom_element : public vdom_cell {
    std::string m_tag;
    json m_attrs;
    std::map<std::string, unsigned> m_events;
    std::vector<vdom> m_children;
    optional<vdom> m_tooltip;
    vdom_element(std::string const & tag, json const & attrs, std::map<std::string, unsigned> events, std::vector<vdom> const & children, optional<vdom> const & tooltip)
      : m_tag(tag), m_attrs(attrs), m_events(events), m_children(children), m_tooltip(tooltip) {}
    optional<std::string> key() override;
    void reconcile(vdom const & old_vdom) override;
    json to_json(list<unsigned> const & route) override;
};

struct vdom_string : public vdom_cell {
    std::string m_val;
    vdom_string(std::string const & val) : m_val(val) {}
    json to_json(list<unsigned> const &) override { return m_val; }
};

class component_instance : public vdom_cell {
    ts_vm_obj const m_component;

    ts_vm_obj m_props;
    optional<ts_vm_obj> m_state;

    std::vector<vdom> m_render;
    // [note] these component instances are owned by one of the vdoms in `m_render`.
    // Should never point to something not owned by m_render.
    std::vector<component_instance *> m_children;
    std::map<unsigned, ts_vm_obj> m_handlers;

    unsigned m_id;
    list<unsigned> m_route;

    bool m_has_rendered;

    vm_obj init(vm_obj const & p, optional<vm_obj> const & s);
    pair<vm_obj, optional<vm_obj>> update(vm_obj const & p, vm_obj const & s, vm_obj const & a);
    vm_obj view(vm_obj const & p, vm_obj const & s);
public:
    void render();
    component_instance(vm_obj const & c, vm_obj const & props, list<unsigned> const & route = list<unsigned>()) : m_component(c), m_props(props), m_route(route) {
      static unsigned count = 0; // [fixme] need to worry about thread safety here. use a global pointer or something.
      m_id = count++;
      m_has_rendered = false;
    }
    json to_json(list<unsigned> const & route) override;
    void reconcile(vdom const & old);
    optional<vm_obj> handleAction(vm_obj const & a);
    optional<vm_obj> handleEvent(list<unsigned> const & route, unsigned handler_id, vm_obj const & eventArgs);
};

/** Iterates, new_elements and old_elements, mutating both (but old_elements is passed by value so that doesn't matter).
 *  new_children is mutated so that they point to vdom components that were successfully reconciled with the old version.
 */
void reconcile_children(std::vector<vdom> & new_elements, std::vector<vdom> const & old_elements);

// void render_attr(vm_obj const & attr, json & attributes, std::map<std::string, unsigned> & events, std::map<unsigned, ts_vm_obj> & handlers);
vdom render_element(vm_obj const & elt, std::vector<component_instance*> & components, std::map<unsigned, ts_vm_obj> & handlers, list<unsigned> const & route);
vdom render_html(vm_obj const & html, std::vector<component_instance*> & components, std::map<unsigned, ts_vm_obj> & handlers, list<unsigned> const & route);
std::vector<vdom> render_html_list(vm_obj const & htmls, std::vector<component_instance*> & components, std::map<unsigned, ts_vm_obj> & handlers, list<unsigned> const & route);

void initialize_widget();
void finalize_widget();

}
