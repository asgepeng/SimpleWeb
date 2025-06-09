#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace AMS {

    enum class Fieldtype
    {
        TextBox,        // <input type="text">
        Password,       // <input type="password">
        Email,          // <input type="email">
        Numeric,        // <input type="number">
        TextArea,       // <textarea>
        Dropdown,       // <select>
        Radio,          // <input type="radio">
        RadioGroup,
        Checkbox,       // <input type="checkbox">
        CheckboxGroup,
        Date,           // <input type="date">
        File,           // <input type="file">
        Hidden          // <input type="hidden">
    };
    struct FieldOption
    {
        std::string value = "";
        std::string text = "";
    };

    struct Formfield
    {
        std::string name = "";                         // Name attribute (important for form submission)
        std::string label = "";                        // Label to be shown
        std::string placeholder = "";                  // Placeholder text
        std::string description = "";                  // Help text or tooltip
        std::string value = "";                        // Default value
        Fieldtype fieldType = Fieldtype::TextBox;      // Type of input
        bool required = false;                         // Validation
        std::vector<FieldOption> options{};              // For dropdown, radio (values only)

        Formfield() = default;

        Formfield(const std::string& name,
            const std::string& label,
            Fieldtype type = Fieldtype::TextBox,
            const std::string& placeholder = "",
            const std::string& value = "",
            bool required = false,
            const std::vector<std::string>& options = {})
            : name(name), label(label), fieldType(type),
            placeholder(placeholder), value(value), required(required)
        {}
    };

    class Form
    {
    public:
        std::string method = "POST";                            // Form method
        std::string actionUrl = "/";                            // Action URL
        std::string formType = "application/x-www-form-urlencoded"; // enctype
        std::string title = "Untitled Form";                    // Optional form title
        std::string description = "";                           // Optional description
        std::string className = "form-control";
        std::vector<Formfield> fields;                          // Fields

        void AddField(const Formfield& field)
        {
            fields.push_back(field);
        }

        void ClearFields()
        {
            fields.clear();
        }
        std::string DrawForm()
        {
            std::ostringstream oss;
            oss << "<form class\"" << className << "\" method=\"" << method << "\" action=\"" << actionUrl << "\" enctype=\"" << formType << "\">\n";

            for (const auto& field : fields)
            {
                // Label
                if (!field.label.empty() &&
                    field.fieldType != Fieldtype::RadioGroup &&
                    field.fieldType != Fieldtype::CheckboxGroup)
                {
                    oss << "<label for=\"" << field.name << "\">" << field.label << "</label>\n";
                }

                switch (field.fieldType)
                {
                case Fieldtype::TextBox:
                case Fieldtype::Password:
                case Fieldtype::Email:
                case Fieldtype::Numeric:
                case Fieldtype::Date:
                case Fieldtype::File:
                case Fieldtype::Hidden:
                {
                    std::string typeStr = "text";
                    switch (field.fieldType)
                    {
                    case Fieldtype::Password: typeStr = "password"; break;
                    case Fieldtype::Email:    typeStr = "email";    break;
                    case Fieldtype::Numeric:  typeStr = "number";   break;
                    case Fieldtype::Date:     typeStr = "date";     break;
                    case Fieldtype::File:     typeStr = "file";     break;
                    case Fieldtype::Hidden:   typeStr = "hidden";   break;
                    default: break;
                    }

                    oss << "<input type=\"" << typeStr
                        << "\" name=\"" << field.name
                        << "\" id=\"" << field.name
                        << "\" value=\"" << field.value << "\"";

                    if (!field.placeholder.empty())
                        oss << " placeholder=\"" << field.placeholder << "\"";

                    if (field.required)
                        oss << " required";

                    oss << ">\n";
                    break;
                }

                case Fieldtype::TextArea:
                {
                    oss << "<textarea name=\"" << field.name
                        << "\" id=\"" << field.name << "\"";

                    if (!field.placeholder.empty())
                        oss << " placeholder=\"" << field.placeholder << "\"";

                    if (field.required)
                        oss << " required";

                    oss << ">" << field.value << "</textarea>\n";
                    break;
                }

                case Fieldtype::Dropdown:
                {
                    oss << "<select name=\"" << field.name
                        << "\" id=\"" << field.name << "\"";

                    if (field.required)
                        oss << " required";

                    oss << ">\n";

                    for (const auto& opt : field.options)
                    {
                        oss << "<option value=\"" << opt.value << "\"";
                        if (opt.value == field.value)
                            oss << " selected";
                        oss << ">" << opt.text << "</option>\n";
                    }

                    oss << "</select>\n";
                    break;
                }

                case Fieldtype::Radio:
                case Fieldtype::Checkbox:
                {
                    std::string typeStr = (field.fieldType == Fieldtype::Radio) ? "radio" : "checkbox";
                    std::string id = field.name + "_" + field.value;

                    oss << "<input type=\"" << typeStr << "\" name=\"" << field.name
                        << "\" id=\"" << id
                        << "\" value=\"" << field.value << "\"";

                    if (field.value == field.value)
                        oss << " checked";

                    if (field.required)
                        oss << " required";

                    oss << ">\n";
                    oss << "<label for=\"" << id << "\">" << field.label << "</label>\n";
                    break;
                }

                case Fieldtype::RadioGroup:
                case Fieldtype::CheckboxGroup:
                {
                    std::string typeStr = (field.fieldType == Fieldtype::RadioGroup) ? "radio" : "checkbox";
                    oss << "<fieldset>\n";
                    if (!field.label.empty())
                        oss << "<legend>" << field.label << "</legend>\n";

                    for (const auto& opt : field.options)
                    {
                        std::string id = field.name + "_" + opt.value;

                        oss << "<input type=\"" << typeStr << "\" name=\"" << field.name
                            << ((typeStr == "checkbox") ? "[]" : "") // array-style for checkbox group
                            << "\" id=\"" << id
                            << "\" value=\"" << opt.value << "\"";

                        // Bisa tambahkan multiple selected values jika kamu pakai vector
                        if (opt.value == field.value)
                            oss << " checked";

                        if (field.required)
                            oss << " required";

                        oss << ">\n";
                        oss << "<label for=\"" << id << "\">" << opt.text << "</label>\n";
                    }

                    oss << "</fieldset>\n";
                    break;
                }

                default:
                    break;
                }

                if (!field.description.empty())
                {
                    oss << "<small>" << field.description << "</small>\n";
                }

                oss << "<br/>\n";
            }

            oss << "<button type=\"submit\">Submit</button>\n";
            oss << "</form>\n";

            return oss.str();
        }
    };

} // namespace AMS
