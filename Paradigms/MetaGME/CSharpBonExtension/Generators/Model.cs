﻿using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace BonExtension.Generators
{
    public class Model : FCO
    {
        new public class Template
        {
            public static readonly string Containment =
@"
        public IEnumerable<{0}> Contained{0}s
        {{
            get
            {{
                MgaObjects children = mgaObject.ChildObjects;
                foreach (MgaObject o in children)
                {{
{1}
                }}
            }}
        }}
";

            public static readonly string ContainmentInner =
@"
                    if (o.MetaBase.Name == ""{0}"")
                        yield return new {1}(o as {2});
";

            public static readonly string ContainmentInterface =
@"
        IEnumerable<{0}> Contained{0}s {{ get; }}
";
        }

        public Model(MGALib.IMgaAtom mgaObject)
            : base(mgaObject)
        {
            className = mgaObject.Name;
            baseInterfaceName = "IModel";

            memberType = "IMgaModel";
        }


        #region Containment
        protected IEnumerable<FCO> Contained
        {
            get
            {
                foreach (MGALib.IMgaObject mgaObject in this.MgaObjects)
                {
                    MGALib.IMgaFCO fco = mgaObject as MGALib.IMgaFCO;
                    foreach (MGALib.IMgaConnPoint conn in fco.PartOfConns)
                    {
                        if (conn.Owner.Meta.Name == "Containment" && conn.ConnRole == "dst")
                        {
                            foreach (MGALib.IMgaConnPoint connOther in conn.Owner.ConnPoints)
                            {
                                if (connOther.ConnRole == "src")
                                {
                                    //connOther.target: contained
                                    if (connOther.target.MetaBase.Name.Contains("Proxy"))
                                    {
                                        if (Object.ProxyCache.ContainsKey(connOther.target.Name))
                                            yield return Object.ElementsByName[Object.ProxyCache[connOther.target.Name]] as FCO;
                                        else
                                            GME.CSharp.BonExtender.Errors.Add("Proxy '" + connOther.target.Name + "' is not found");
                                    }
                                    else
                                    {
                                        if (Object.ElementsByName.ContainsKey(connOther.target.Name))
                                            yield return Object.ElementsByName[connOther.target.Name] as FCO;
                                        else
                                        {
                                            //todo
                                            GME.CSharp.BonExtender.Errors.Add(connOther.target.Name + " is not found");
                                            //throw new Exception(connOther.target.Name + " is not cached");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public string GenerateContainments(ref List<string> names, ref StringBuilder forInterface)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(generateOwnContainments(ref names, ref forInterface));

            //genarate parents' attributes:
            foreach (FCO parent in this.Parents)
            {
                if (parent is Model)
                    sb.Append((parent as Model).GenerateContainments(ref names, ref forInterface));
            }

            return sb.ToString();
        }
        private string generateOwnContainments(ref List<string> names, ref StringBuilder forInterface)
        {
            StringBuilder sb = new StringBuilder();
            foreach (FCO cont in Contained)
            {
                if (!names.Contains(cont.className))
                {
                    sb.Append(generateContainment(cont));
                    if (this.HasChildren)
                    {
                        forInterface.Append(generateContainmentForInterface(cont));
                    }
                    names.Add(cont.className);
                }
            }
            return sb.ToString();
        }
        private string generateContainment(FCO current)
        {
            StringBuilder inner = new StringBuilder();

            //FCO current = Object.ElementsByName[typename] as FCO;

            inner.AppendFormat(Model.Template.ContainmentInner, current.className, current.ProperClassName, current.memberType);
            if (current.HasChildren)
            {
                //and add all of the children
                foreach (FCO child in current.ChildrenRecursive)
                {
                    inner.AppendFormat(Model.Template.ContainmentInner, child.className, child.ProperClassName, child.memberType);
                }
            }

            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(Model.Template.Containment, current.className, inner.ToString());

            return sb.ToString();
        }
        private string generateContainmentForInterface(FCO current)
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(Model.Template.ContainmentInterface, current.memberType);

            return sb.ToString();
        }
        #endregion

        public override string GenerateClass()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(Init());

            List<string> rels = new List<string>();
            List<string> attrs = new List<string>();
            List<string> conns = new List<string>();
            List<string> conts = new List<string>();
            List<string> crnews = new List<string>();

            StringBuilder sbAttrib = new StringBuilder();
            StringBuilder sbContain = new StringBuilder();
            StringBuilder sbConns = new StringBuilder();
            StringBuilder sbRels = new StringBuilder();

            string baseInterfaces = (this.HasChildren) ? className : baseInterfaceName;

            foreach (FCO parent in this.Parents)
            {
                baseInterfaces = baseInterfaces + ", " + parent.Name;
            }

            sb.AppendFormat(
                FCO.Template.Class,
                namespaceName,
                (this.HasChildren) ? className + "Impl" : className,
                baseInterfaces,
                memberType,
                GenerateCommon(),
                GenerateAttributes(ref attrs, ref sbAttrib),
                GenerateConnections(ref conns, ref sbConns),
                GenerateRelationships(ref rels, ref sbRels),
                GenerateContainments(ref conts, ref sbContain),
                "IMgaMeta" + memberType.Substring(4),
                className,
                GenerateCreateNews(ref crnews,this));

            baseInterfaces = baseInterfaceName;

            foreach (FCO parent in this.Parents)
            {
                baseInterfaces = baseInterfaces + ", " + parent.Name;
            }

            if (this.HasChildren)
            {
                //have to generate interface as well
                sb.AppendFormat(
                FCO.Template.Interface,
                namespaceName,
                className,
                baseInterfaces,
                memberType,
                GenerateCommon(),
                sbAttrib.ToString(),
                sbConns.ToString(),
                sbRels.ToString(),
                sbContain.ToString());
            }

            return sb.ToString();
        }
    }
}
        