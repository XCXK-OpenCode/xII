//
// Created by qpwser from XCXK on 7/10/26
//

#pragma once

#include "Base.h"
#include "Renderer/Base.h"
#include "Shape/Base.h"

#include <memory>
#include <vector>

namespace xII::Item {
    class General : public Base {
    protected:
        std::shared_ptr<Shape::Base> _shape;
        std::vector<std::shared_ptr<Renderer::Base>> _renderers;

    public:
        General(const General&) = default;
        General(const std::shared_ptr<Shape::Base>& shape, const std::vector<std::shared_ptr<Renderer::Base>>& renderers)
            : _shape(shape),
              _renderers(renderers) {}

        ~General() override = default;

        std::shared_ptr<Shape::Base> getShape() const { return _shape; }
        void setShape(const std::shared_ptr<Shape::Base>& shape) { _shape = shape; }

        std::vector<std::shared_ptr<Renderer::Base>> getRenderers() const { return _renderers; }
        std::vector<std::shared_ptr<Renderer::Base>>& getRenderersR() { return _renderers; }
        const std::vector<std::shared_ptr<Renderer::Base>>& getRenderersR() const { return _renderers; }
        void setRenderers(const std::vector<std::shared_ptr<Renderer::Base>>& renderers) { _renderers = renderers; }

        void Render(Camera::Standard& camera) const override;
        void Render(Camera::Outline& camera) const override;
    };
} // namespace xII::Item
