#ifndef NONLINFIT_PLANE_JOINTTERMIMPLEMENTATION_H
#define NONLINFIT_PLANE_JOINTTERMIMPLEMENTATION_H

#include "nonlinfit/plane/Term.h"

#include "nonlinfit/derive_by.hpp"
#include "nonlinfit/get_variable.hpp"
#include "nonlinfit/parameter_is_negligible.hpp"
#include "nonlinfit/plane/Joint.h"
#include "nonlinfit/plane/JointData.h"
#include "nonlinfit/set_variable.hpp"

namespace nonlinfit {
namespace plane {

template <typename Lambda, typename Tag>
class JointTermImplementation : public Term<Tag> {
    typedef typename Tag::Number Number;
    typedef typename get_evaluator<Lambda, Tag>::type Evaluator;

    typedef typename Term<Tag>::ValueVector ValueVector;
    typedef typename Term<Tag>::JacobianBlock JacobianBlock;
    typedef typename Term<Tag>::JacobianRowBlock JacobianRowBlock;
    typedef typename Term<Tag>::PositionBlock PositionBlock;
    typedef typename Term<Tag>::ConstPositionBlock ConstPositionBlock;

    Lambda& lambda;
    Evaluator evaluator;

  public:
    JointTermImplementation(Lambda& lambda)
        : Term<Tag>(boost::mpl::size<typename Lambda::Variables>::value,
                    boost::mpl::size<typename Lambda::Variables>::value),
          lambda(lambda),
          evaluator(lambda) {}

    bool prepare_iteration(const typename Tag::Data& inputs) OVERRIDE {
        return evaluator.prepare_iteration(inputs);
    }

    virtual bool prepare_disjoint_iteration(
            const typename Tag::Data&,
            JacobianBlock x_jacobian) {
        throw std::logic_error("Tried to run disjoint operations on joint data");
    }

    void evaluate_chunk(
            const typename Tag::Data::Input& data,
            ValueVector& values,
            JacobianBlock jacobian_block) OVERRIDE {
        evaluator.prepare_chunk(data);
        evaluator.add_value(values);
        Jacobian<typename Lambda::Variables>::compute(evaluator, jacobian_block);
    }

    void evaluate_disjoint_chunk(
        const typename Tag::Data::Input& data,
        ValueVector& values,
        JacobianRowBlock jacobian) OVERRIDE {
        throw std::logic_error("Tried to run disjoint operations on joint data");
    }

    void set_position(ConstPositionBlock position) OVERRIDE {
	set_variable::read_vector(position, lambda);
    }

    void get_position(PositionBlock position) const OVERRIDE {
	get_variable::fill_vector(lambda, position);
    }

    Eigen::VectorXi get_reduction_term() const {
        throw std::logic_error("Tried to run disjoint operations on joint data");
    }

    bool step_is_negligible(
            ConstPositionBlock from,
            ConstPositionBlock to) const OVERRIDE {
        return parameter_is_negligible().all(lambda, from, to);
    }
};

}
}

#endif
